/**********************************************************************
  RandomDock -- A tool for analysing a matrix-substrate docking problem

  Copyright (C) 2009-2011 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <randomdock/ui/tab_plot.h>

#include <randomdock/randomdock.h>
#include <randomdock/structures/scene.h>
#include <randomdock/ui/dialog.h>

#include <globalsearch/macros.h>
#include <globalsearch/queuemanager.h>
#include <globalsearch/tracker.h>

#include <avogadro/glwidget.h>
#include <avogadro/primitive.h>
#include <avogadro/primitivelist.h>

#include <QReadWriteLock>
#include <QSettings>

using namespace std;
using namespace Avogadro;
using namespace GlobalSearch;

namespace RandomDock {

TabPlot::TabPlot(RandomDockDialog* parent, RandomDock* p)
  : AbstractTab(parent, p), m_plot_mutex(new QReadWriteLock()), m_plotObject(0)
{
  ui.setupUi(m_tab_widget);

  // Plot setup
  ui.plot_plot->setAntialiasing(true);
  updatePlot();

  // dialog connections
  connect(m_dialog, SIGNAL(moleculeChanged(GlobalSearch::Structure*)), this,
          SLOT(highlightStructure(GlobalSearch::Structure*)));
  connect(m_opt, SIGNAL(sessionStarted()), this, SLOT(populateStructureList()));
  connect(m_opt, SIGNAL(readOnlySessionStarted()), this,
          SLOT(populateStructureList()));
  connect(m_dialog->getGLWidget(), SIGNAL(mouseRelease(QMouseEvent*)), this,
          SLOT(updatePlot()));

  // Plot connections
  connect(ui.push_refresh, SIGNAL(clicked()), this, SLOT(refreshPlot()));
  connect(ui.combo_xAxis, SIGNAL(currentIndexChanged(int)), this,
          SLOT(refreshPlot()));
  connect(ui.combo_yAxis, SIGNAL(currentIndexChanged(int)), this,
          SLOT(refreshPlot()));
  connect(ui.combo_plotType, SIGNAL(currentIndexChanged(int)), this,
          SLOT(refreshPlot()));
  connect(ui.combo_distHistStructure, SIGNAL(currentIndexChanged(int)), this,
          SLOT(refreshPlot()));
  connect(ui.combo_distHistStructure, SIGNAL(currentIndexChanged(int)), this,
          SLOT(selectStructureFromIndex(int)));
  connect(ui.cb_labelPoints, SIGNAL(toggled(bool)), this, SLOT(updatePlot()));
  connect(ui.combo_labelType, SIGNAL(currentIndexChanged(int)), this,
          SLOT(updatePlot()));
  connect(ui.cb_showDuplicates, SIGNAL(toggled(bool)), this,
          SLOT(updatePlot()));
  connect(ui.cb_showIncompletes, SIGNAL(toggled(bool)), this,
          SLOT(updatePlot()));
  connect(ui.plot_plot, SIGNAL(pointClicked(PlotPoint*)), this,
          SLOT(selectStructureFromPlot(PlotPoint*)));
  connect(ui.plot_plot, SIGNAL(pointClicked(PlotPoint*)), this,
          SLOT(lockClearAndSelectPoint(PlotPoint*)));
  connect(m_opt, SIGNAL(refreshAllStructureInfo()), this, SLOT(refreshPlot()));
  connect(m_opt, SIGNAL(refreshAllStructureInfo()), this,
          SLOT(populateStructureList()));
  connect(m_opt->queue(), SIGNAL(structureUpdated(GlobalSearch::Structure*)),
          this, SLOT(populateStructureList()));
  connect(m_opt->tracker(), SIGNAL(newStructureAdded(GlobalSearch::Structure*)),
          this, SLOT(populateStructureList()));
  connect(m_opt->queue(), SIGNAL(structureUpdated(GlobalSearch::Structure*)),
          this, SLOT(updatePlot()));
  connect(m_opt->tracker(), SIGNAL(newStructureAdded(GlobalSearch::Structure*)),
          this, SLOT(updatePlot()));

  initialize();
}

TabPlot::~TabPlot()
{
  delete m_plot_mutex;
  // m_plotObject is deleted by the PlotWidget
}

void TabPlot::writeSettings(const QString& filename)
{
  SETTINGS(filename);

  settings->beginGroup("randomdock/plot/");
  const int version = 1;
  settings->setValue("version", version);

  settings->setValue("x_label", ui.combo_xAxis->currentIndex());
  settings->setValue("y_label", ui.combo_yAxis->currentIndex());
  settings->setValue("showDuplicates", ui.cb_showDuplicates->isChecked());
  settings->setValue("showIncompletes", ui.cb_showIncompletes->isChecked());
  settings->setValue("labelPoints", ui.cb_labelPoints->isChecked());
  settings->setValue("labelType", ui.combo_labelType->currentIndex());
  settings->setValue("plotType", ui.combo_plotType->currentIndex());
  settings->endGroup();

  DESTROY_SETTINGS(filename);
}

void TabPlot::readSettings(const QString& filename)
{
  SETTINGS(filename);
  settings->beginGroup("randomdock/plot/");
  int loadedVersion = settings->value("version", 0).toInt();

  ui.combo_xAxis->setCurrentIndex(
    settings->value("x_label", Structure_T).toInt());
  ui.combo_yAxis->setCurrentIndex(settings->value("y_label", Energy_T).toInt());
  ui.cb_showDuplicates->setChecked(
    settings->value("showDuplicates", false).toBool());
  ui.cb_showIncompletes->setChecked(
    settings->value("showIncompletes", false).toBool());
  ui.cb_labelPoints->setChecked(settings->value("labelPoints", false).toBool());
  ui.combo_labelType->setCurrentIndex(
    settings->value("labelType", Energy_L).toInt());
  ui.combo_plotType->setCurrentIndex(
    settings->value("plotType", Trend_PT).toInt());
  settings->endGroup();

  // Update config data
  switch (loadedVersion) {
    case 0:
    case 1:
    default:
      break;
  }
}

void TabPlot::updateGUI()
{
  switch (ui.combo_plotType->currentIndex()) {
    case Trend_PT:
    default:
      ui.gb_distance->setEnabled(false);
      ui.gb_trend->setEnabled(true);
      break;
    case DistHist_PT:
      ui.gb_trend->setEnabled(false);
      ui.gb_distance->setEnabled(true);
      break;
  }
}

void TabPlot::disconnectGUI()
{
  ui.push_refresh->disconnect();
  ui.combo_xAxis->disconnect();
  ui.combo_yAxis->disconnect();
  ui.combo_plotType->disconnect();
  ui.combo_distHistStructure->disconnect();
  ui.cb_labelPoints->disconnect();
  ui.combo_labelType->disconnect();
  ui.cb_showDuplicates->disconnect();
  ui.cb_showIncompletes->disconnect();
  ui.plot_plot->disconnect();
  this->disconnect();
  disconnect(m_dialog, 0, this, 0);
  disconnect(m_opt, 0, this, 0);
}

void TabPlot::lockClearAndSelectPoint(PlotPoint* pp)
{
  m_plot_mutex->lockForRead();
  ui.plot_plot->clearAndSelectPoint(pp);
  m_plot_mutex->unlock();
}

void TabPlot::refreshPlot()
{
  // Reset connections
  ui.plot_plot->disconnect(this);

  // Set up plot object
  updatePlot();

  // Reset limits and connections.
  if (ui.combo_plotType->currentIndex() != DistHist_PT) {
    ui.plot_plot->scaleLimits();
    connect(ui.plot_plot, SIGNAL(pointClicked(PlotPoint*)), this,
            SLOT(selectStructureFromPlot(PlotPoint*)));
    connect(ui.plot_plot, SIGNAL(pointClicked(PlotPoint*)), this,
            SLOT(lockClearAndSelectPoint(PlotPoint*)));
  }
}

void TabPlot::updatePlot()
{
  updateGUI();
  if (!m_opt)
    return;

  // Make sure we have structures!
  if (m_opt->tracker()->size() == 0) {
    return;
  }

  // Lock plot mutex
  m_plot_mutex->lockForWrite();

  // Turn off updates
  ui.plot_plot->blockSignals(true);

  switch (ui.combo_plotType->currentIndex()) {
    case Trend_PT:
      plotTrends();
      break;
    case DistHist_PT:
      plotDistHist();
      break;
  }

  ui.plot_plot->blockSignals(false);

  m_plot_mutex->unlock();
}

void TabPlot::plotTrends()
{
  // Store current limits for later
  QRectF oldDataRect = ui.plot_plot->dataRect();

  // Clear old data...
  ui.plot_plot->resetPlot();

  m_plotObject =
    new PlotObject(Qt::red, PlotObject::Points, 4, PlotObject::Triangle);

  double x, y;
  int ind;
  Scene* scene;
  PlotPoint* pp;
  // Load config settings:
  bool labelPoints = ui.cb_labelPoints->isChecked();
  bool showDuplicates = ui.cb_showDuplicates->isChecked();
  bool showIncompletes = ui.cb_showIncompletes->isChecked();
  LabelTypes labelType = LabelTypes(ui.combo_labelType->currentIndex());
  PlotAxes xAxis = PlotAxes(ui.combo_xAxis->currentIndex());
  PlotAxes yAxis = PlotAxes(ui.combo_yAxis->currentIndex());

  for (int i = 0; i < m_opt->tracker()->size(); i++) {
    x = y = 0;
    scene = qobject_cast<Scene*>(m_opt->tracker()->at(i));
    QReadLocker sceneLocker(scene->lock());
    // Don't plot removed scenes or those who have not completed
    // their first optstep (energy < 1e-50).
    if ((scene->getStatus() != Scene::Optimized && !showIncompletes)) {
      if (!(scene->getStatus() == Scene::Duplicate && showDuplicates)) {
        continue;
      }
    }

    if (scene->getStatus() == Scene::Duplicate && !showDuplicates) {
      continue;
    }

    if (scene->getStatus() == Scene::Killed ||
        scene->getStatus() == Scene::Removed ||
        fabs(scene->getEnergy()) <= 1e-50) {
      continue;
    }

    // Get X/Y data
    for (int j = 0; j < 2; j++) { // 0 = x, 1 = y
      switch (j) {
        case 0:
          ind = xAxis;
          break;
        default:
          ind = yAxis;
          break;
      }

      switch (ind) {
        case Structure_T:
          switch (j) {
            case 0:
              x = i + 1;
              break;
            default:
              y = i + 1;
              break;
          }
          break;
        case Energy_T:
          // Skip scenes that don't have energy set
          if (scene->getEnergy() == 0.0)
            continue;
          switch (j) {
            case 0:
              x = scene->getEnergy();
              break;
            default:
              y = scene->getEnergy();
              break;
          }
          break;
      }
    }
    pp = m_plotObject->addPoint(x, y);

    // Store index for later lookup
    pp->setCustomData(i);

    // Set point label if requested
    if (labelPoints) {
      switch (labelType) {
        case Index_L:
        default:
          pp->setLabel(QString::number(scene->getIndex()));
          break;
        case Energy_L:
          pp->setLabel(QString::number(scene->getEnergy(), 'g', 5));
          break;
      }
    }
  }

  // Set axis labels
  for (int j = 0; j < 2; j++) { // 0 = x, 1 = y
    switch (j) {
      case 0:
        ind = ui.combo_xAxis->currentIndex();
        break;
      default:
        ind = ui.combo_yAxis->currentIndex();
        break;
    }

    QString label;
    switch (ind) {
      case Structure_T:
        label = tr("Structure");
        switch (j) {
          case 0:
            ui.plot_plot->axis(PlotWidget::BottomAxis)->setLabel(label);
            break;
          default:
            ui.plot_plot->axis(PlotWidget::LeftAxis)->setLabel(label);
            break;
        }
        break;
      case Energy_T:
        label = tr("Energy (eV)");
        switch (j) {
          case 0:
            ui.plot_plot->axis(PlotWidget::BottomAxis)->setLabel(label);
            break;
          default:
            ui.plot_plot->axis(PlotWidget::LeftAxis)->setLabel(label);
            break;
        }
        break;
    }
  }

  ui.plot_plot->addPlotObject(m_plotObject);

  // Do not scale if m_plotObject is empty.
  // If we have one point, set limits to something appropriate:
  if (m_plotObject->points().size() == 1) {
    double x = m_plotObject->points().at(0)->x();
    double y = m_plotObject->points().at(0)->y();
    ui.plot_plot->setDefaultLimits(x - 1, x + 1, y + 1, y - 1);
  }
  // For multiple points, let plotwidget handle it.
  else if (m_plotObject->points().size() >= 2) {
    // run scaleLimits to set the default limits, but then set the
    // limits to the previous region
    ui.plot_plot->scaleLimits();
    ui.plot_plot->setLimits(
      oldDataRect.left(), oldDataRect.right(),
      oldDataRect.top(),     // These are backwards from intuition,
      oldDataRect.bottom()); // but that's how Qt works...
  }
}

void TabPlot::plotDistHist()
{
  // Clear old data...
  ui.plot_plot->resetPlot();

  // Initialize vars
  m_plotObject = new PlotObject(Qt::red, PlotObject::Bars);
  double x, y;
  PlotPoint* pp;
  QList<double> d, f, f_temp;

  // Determine structure
  int ind = ui.combo_distHistStructure->currentIndex();
  if (ind < 0 || ind > m_opt->tracker()->size() - 1) {
    ind = 0;
  }
  Scene* scene = qobject_cast<Scene*>(m_opt->tracker()->at(ind));

  // Determine selected atoms, if any
  QList<Primitive*> selected =
    m_dialog->getGLWidget()->selectedPrimitives().subList(Primitive::AtomType);

  // Get histogram
  // If no atoms selected...
  if (selected.size() == 0) {
    scene->lock()->lockForRead();
    scene->getDefaultHistogram(&d, &f);
    scene->lock()->unlock();
  }
  // If atoms are selected:
  else {
    scene->lock()->lockForRead();
    for (int i = 0; i < selected.size(); i++) {
      scene->generateIADHistogram(&d, &f_temp, 0, 15, .1,
                                  qobject_cast<Atom*>(selected.at(i)));
      if (f.isEmpty()) {
        f = f_temp;
      } else {
        for (int j = 0; j < f.size(); j++) {
          f[j] += f_temp[j];
        }
      }
    }
    scene->lock()->unlock();
  }

  // Populate plot object
  for (int i = 0; i < d.size(); i++) {
    x = d.at(i);
    y = f.at(i);
    pp = m_plotObject->addPoint(x, y);
  }

  ui.plot_plot->axis(PlotWidget::BottomAxis)->setLabel(tr("Distance"));
  ui.plot_plot->axis(PlotWidget::LeftAxis)->setLabel(tr("Count"));

  ui.plot_plot->addPlotObject(m_plotObject);

  // Set default limits
  ui.plot_plot->scaleLimits();
  ui.plot_plot->setDefaultLimits(0, 8, 0, ui.plot_plot->dataRect().bottom());
}

void TabPlot::populateStructureList()
{
  int ind = ui.combo_distHistStructure->currentIndex();
  ui.combo_distHistStructure->blockSignals(true);
  ui.combo_distHistStructure->clear();
  QList<Structure*>* structures = m_opt->tracker()->list();
  Scene* scene;
  QString s;
  for (int i = 0; i < structures->size(); i++) {
    scene = qobject_cast<Scene*>(structures->at(i));
    scene->lock()->lockForRead();
    s.clear();
    // index:
    s.append(QString::number(i) + " ");
    // disposition
    switch (scene->getStatus()) {
      case Scene::Optimized:
        s.append(" (o)");
        break;
      case Scene::StepOptimized:
      case Scene::WaitingForOptimization:
      case Scene::Submitted:
      case Scene::InProcess:
      case Scene::Empty:
      case Scene::Restart:
      case Scene::Updating:
        s.append(" (p)");
        break;
      case Scene::Error:
        s.append(" (e)");
        break;
      case Scene::Killed:
      case Scene::Removed:
        s.append(" (k)");
        break;
      case Scene::Duplicate:
        s.append(" (d)");
        break;
      default:
        break;
    }
    ui.combo_distHistStructure->addItem(s);
    scene->lock()->unlock();
  }
  if (ind == -1)
    ind = 0;
  ui.combo_distHistStructure->setCurrentIndex(ind);
  ui.combo_distHistStructure->blockSignals(false);
}

void TabPlot::selectStructureFromPlot(PlotPoint* pp)
{
  if (!pp)
    return;
  int index = pp->customData().toInt();
  selectStructureFromIndex(index);
}

void TabPlot::selectStructureFromIndex(int index)
{
  if (index < 0 || index > m_opt->tracker()->size() - 1) {
    index = 0;
  }
  if (m_opt->tracker()->size() == 0) {
    return;
  }
  emit moleculeChanged(m_opt->tracker()->at(index));
}

void TabPlot::highlightStructure(Structure* structure)
{
  // Bail out if there is no plotobject in memory
  if (!m_plotObject)
    return;
  QReadLocker plotLocker(m_plot_mutex);
  structure->lock()->lockForRead();
  uint id = structure->getIDNumber();
  structure->lock()->unlock();
  int ind;
  Structure* tstructure;
  for (int i = 0; i < m_opt->tracker()->size(); i++) {
    tstructure = m_opt->tracker()->at(i);
    tstructure->lock()->lockForRead();
    uint tid = tstructure->getIDNumber();
    tstructure->lock()->unlock();
    if (tid == id) {
      ind = i;
      break;
    }
  }
  if (ui.combo_plotType->currentIndex() == Trend_PT) {
    PlotPoint* pp;
    bool found = false;
    QList<PlotPoint*> points(m_plotObject->points());
    for (int i = 0; i < points.size(); i++) {
      pp = points.at(i);
      if (pp->customData().toInt() == ind) {
        ui.plot_plot->blockSignals(true);
        ui.plot_plot->clearAndSelectPoint(pp);
        ui.plot_plot->blockSignals(false);
        found = true;
        break;
      }
    }
    if (!found) {
      // If not found, clear selection
      ui.plot_plot->blockSignals(true);
      ui.plot_plot->clearSelection();
      ui.plot_plot->blockSignals(false);
    }
  }
  // Update structure combo
  plotLocker.unlock();
  ui.combo_distHistStructure->blockSignals(true);
  ui.combo_distHistStructure->setCurrentIndex(ind);
  ui.combo_distHistStructure->blockSignals(false);
  if (ui.combo_plotType->currentIndex() == DistHist_PT) {
    refreshPlot();
  }
}
}
