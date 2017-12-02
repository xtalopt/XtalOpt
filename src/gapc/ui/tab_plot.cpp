/**********************************************************************
  TabPlot - Tab with interactive plot

  Copyright (C) 2009-2011 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <gapc/ui/tab_plot.h>

#include <gapc/gapc.h>
#include <gapc/structures/protectedcluster.h>
#include <gapc/ui/dialog.h>

#include <globalsearch/macros.h>
#include <globalsearch/queuemanager.h>
#include <globalsearch/tracker.h>

#include <avogadro/glwidget.h>
#include <avogadro/primitive.h>
#include <avogadro/primitivelist.h>

#include <QReadWriteLock>
#include <QSettings>

using namespace Avogadro;
using namespace GlobalSearch;

namespace GAPC {

TabPlot::TabPlot(GAPCDialog* parent, OptGAPC* p)
  : AbstractTab(parent, p), m_plot_mutex(new QReadWriteLock()), m_plotObject(0)
{
  ui.setupUi(m_tab_widget);

  // Plot setup
  ui.plot_plot->setAntialiasing(true);
  updatePlot();

  // dialog connections
  connect(m_dialog, SIGNAL(moleculeChanged(GlobalSearch::Structure*)), this,
          SLOT(highlightPC(GlobalSearch::Structure*)));
  connect(m_opt, SIGNAL(sessionStarted()), this, SLOT(populatePCList()));
  connect(m_opt, SIGNAL(readOnlySessionStarted()), this,
          SLOT(populatePCList()));
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
  connect(ui.combo_distHistPC, SIGNAL(currentIndexChanged(int)), this,
          SLOT(refreshPlot()));
  connect(ui.combo_distHistPC, SIGNAL(currentIndexChanged(int)), this,
          SLOT(selectMoleculeFromIndex(int)));
  connect(ui.cb_labelPoints, SIGNAL(toggled(bool)), this, SLOT(updatePlot()));
  connect(ui.combo_labelType, SIGNAL(currentIndexChanged(int)), this,
          SLOT(updatePlot()));
  connect(ui.cb_showDuplicates, SIGNAL(toggled(bool)), this,
          SLOT(updatePlot()));
  connect(ui.cb_showIncompletes, SIGNAL(toggled(bool)), this,
          SLOT(updatePlot()));
  connect(ui.plot_plot, SIGNAL(pointClicked(PlotPoint*)), this,
          SLOT(selectMoleculeFromPlot(PlotPoint*)));
  connect(ui.plot_plot, SIGNAL(pointClicked(PlotPoint*)), this,
          SLOT(lockClearAndSelectPoint(PlotPoint*)));
  connect(m_opt, SIGNAL(refreshAllStructureInfo()), this, SLOT(refreshPlot()));
  connect(m_opt, SIGNAL(refreshAllStructureInfo()), this,
          SLOT(populatePCList()));
  connect(m_opt->queue(), SIGNAL(structureUpdated(GlobalSearch::Structure*)),
          this, SLOT(populatePCList()));
  connect(m_opt->tracker(), SIGNAL(newStructureAdded(GlobalSearch::Structure*)),
          this, SLOT(populatePCList()));
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
  const int version = 1;
  settings->beginGroup("gapc/plot/");
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
  settings->beginGroup("gapc/plot/");
  int loadedVersion = settings->value("version", 0).toInt();
  ui.combo_xAxis->setCurrentIndex(
    settings->value("x_label", Structure_T).toInt());
  ui.combo_yAxis->setCurrentIndex(
    settings->value("y_label", Enthalpy_T).toInt());
  ui.cb_showDuplicates->setChecked(
    settings->value("showDuplicates", false).toBool());
  ui.cb_showIncompletes->setChecked(
    settings->value("showIncompletes", false).toBool());
  ui.cb_labelPoints->setChecked(settings->value("labelPoints", false).toBool());
  ui.combo_labelType->setCurrentIndex(
    settings->value("labelType", Structure_L).toInt());
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
  ui.combo_distHistPC->disconnect();
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
  // qDebug() << "TabPlot::refreshPlot() called";

  // Reset connections
  ui.plot_plot->disconnect(this);

  // Set up plot object
  updatePlot();

  // Reset limits and connections.
  if (ui.combo_plotType->currentIndex() != DistHist_PT) {
    ui.plot_plot->scaleLimits();
    connect(ui.plot_plot, SIGNAL(pointClicked(PlotPoint*)), this,
            SLOT(selectMoleculeFromPlot(PlotPoint*)));
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
  ProtectedCluster* pc;
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
    pc = qobject_cast<ProtectedCluster*>(m_opt->tracker()->at(i));
    QReadLocker pcLocker(pc->lock());
    // Don't plot removed structures or those who have not completed their first
    // INCAR.
    if ((pc->getStatus() != ProtectedCluster::Optimized && !showIncompletes)) {
      if (!(pc->getStatus() == ProtectedCluster::Duplicate && showDuplicates)) {
        continue;
      }
    }

    if (pc->getStatus() == ProtectedCluster::Duplicate && !showDuplicates) {
      continue;
    }

    if (pc->getStatus() == ProtectedCluster::Killed ||
        pc->getStatus() == ProtectedCluster::Removed ||
        fabs(pc->getEnthalpy()) <= 1e-50) {
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
        case Generation_T:
          switch (j) {
            case 0:
              x = pc->getGeneration();
              break;
            default:
              y = pc->getGeneration();
              break;
          }
          break;
        case Enthalpy_T:
          // Skip pcs that don't have enthalpy/energy set
          if (pc->getEnergy() == 0.0 && !pc->hasEnthalpy())
            continue;
          switch (j) {
            case 0:
              x = pc->getEnthalpy();
              break;
            default:
              y = pc->getEnthalpy();
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
        case Enthalpy_L:
          pp->setLabel(QString::number(pc->getEnthalpy(), 'g', 5));
          break;
        case Generation_L:
          pp->setLabel(QString::number(pc->getGeneration()));
          break;
        case Structure_L:
          pp->setLabel(QString::number(i));
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
      case Generation_T:
        label = tr("Generation");
        switch (j) {
          case 0:
            ui.plot_plot->axis(PlotWidget::BottomAxis)->setLabel(label);
            break;
          default:
            ui.plot_plot->axis(PlotWidget::LeftAxis)->setLabel(label);
            break;
        }
        break;
      case Enthalpy_T:
        label = tr("Enthalpy (eV)");
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

  // Determine pc
  int ind = ui.combo_distHistPC->currentIndex();
  if (ind < 0 || ind > m_opt->tracker()->size() - 1) {
    ind = 0;
  }
  ProtectedCluster* pc =
    qobject_cast<ProtectedCluster*>(m_opt->tracker()->at(ind));

  // Determine selected atoms, if any
  QList<Primitive*> selected =
    m_dialog->getGLWidget()->selectedPrimitives().subList(Primitive::AtomType);

  // Get histogram
  // If no atoms selected...
  if (selected.size() == 0) {
    pc->lock()->lockForRead();
    pc->getDefaultHistogram(&d, &f);
    pc->lock()->unlock();
  }
  // If atoms are selected:
  else {
    pc->lock()->lockForRead();
    for (int i = 0; i < selected.size(); i++) {
      pc->generateIADHistogram(&d, &f_temp, 0, 15, .1,
                               qobject_cast<Atom*>(selected.at(i)));
      if (f.isEmpty()) {
        f = f_temp;
      } else {
        for (int j = 0; j < f.size(); j++) {
          f[j] += f_temp[j];
        }
      }
    }
    pc->lock()->unlock();
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

void TabPlot::populatePCList()
{
  int ind = ui.combo_distHistPC->currentIndex();
  ui.combo_distHistPC->blockSignals(true);
  ui.combo_distHistPC->clear();
  const QList<Structure*>* structures = m_opt->tracker()->list();
  ProtectedCluster* pc;
  QString s;
  for (int i = 0; i < structures->size(); i++) {
    pc = qobject_cast<ProtectedCluster*>(structures->at(i));
    pc->lock()->lockForRead();
    s.clear();
    // index:
    s.append(QString::number(i) + ": ");
    // generation and ID:
    s.append(pc->getIDString());
    // disposition
    switch (pc->getStatus()) {
      case ProtectedCluster::Optimized:
        s.append(" (o)");
        break;
      case ProtectedCluster::StepOptimized:
      case ProtectedCluster::WaitingForOptimization:
      case ProtectedCluster::Submitted:
      case ProtectedCluster::InProcess:
      case ProtectedCluster::Empty:
      case ProtectedCluster::Restart:
      case ProtectedCluster::Updating:
        s.append(" (p)");
        break;
      case ProtectedCluster::Error:
        s.append(" (e)");
        break;
      case ProtectedCluster::Killed:
      case ProtectedCluster::Removed:
        s.append(" (k)");
        break;
      case ProtectedCluster::Duplicate:
        s.append(" (d)");
        break;
      default:
        break;
    }
    ui.combo_distHistPC->addItem(s);
    pc->lock()->unlock();
  }
  if (ind == -1)
    ind = 0;
  ui.combo_distHistPC->setCurrentIndex(ind);
  ui.combo_distHistPC->blockSignals(false);
}

void TabPlot::selectMoleculeFromPlot(PlotPoint* pp)
{
  if (!pp)
    return;
  int index = pp->customData().toInt();
  selectMoleculeFromIndex(index);
}

void TabPlot::selectMoleculeFromIndex(int index)
{
  if (index < 0 || index > m_opt->tracker()->size() - 1) {
    index = 0;
  }
  if (m_opt->tracker()->size() == 0) {
    return;
  }
  emit moleculeChanged(m_opt->tracker()->at(index));
}

void TabPlot::highlightPC(Structure* s)
{
  ProtectedCluster* pc = qobject_cast<ProtectedCluster*>(s);

  // Bail out if there is no plotobject in memory
  if (!m_plotObject)
    return;
  QReadLocker plotLocker(m_plot_mutex);
  pc->lock()->lockForRead();
  uint gen = pc->getGeneration();
  uint id = pc->getIDNumber();
  pc->lock()->unlock();
  int ind;
  ProtectedCluster* tpc;
  for (int i = 0; i < m_opt->tracker()->size(); i++) {
    tpc = qobject_cast<ProtectedCluster*>(m_opt->tracker()->at(i));
    tpc->lock()->lockForRead();
    uint tgen = tpc->getGeneration();
    uint tid = tpc->getIDNumber();
    tpc->lock()->unlock();
    if (tgen == gen && tid == id) {
      ind = i;
      break;
    }
  }
  if (ui.combo_plotType->currentIndex() == Trend_PT) {
    PlotPoint* pp;
    bool found = false;
    QList<PlotPoint*>* points = new QList<PlotPoint*>(m_plotObject->points());
    for (int i = 0; i < points->size(); i++) {
      pp = points->at(i);
      if (pp->customData().toInt() == ind) {
        ui.plot_plot->blockSignals(true);
        ui.plot_plot->clearAndSelectPoint(pp);
        ui.plot_plot->blockSignals(false);
        found = true;
        break;
      }
    }
    delete points;
    if (!found) {
      // If not found, clear selection
      ui.plot_plot->blockSignals(true);
      ui.plot_plot->clearSelection();
      ui.plot_plot->blockSignals(false);
    }
  }
  // Update structure combo
  plotLocker.unlock();
  ui.combo_distHistPC->blockSignals(true);
  ui.combo_distHistPC->setCurrentIndex(ind);
  ui.combo_distHistPC->blockSignals(false);
  if (ui.combo_plotType->currentIndex() == DistHist_PT) {
    refreshPlot();
  }
}
}
