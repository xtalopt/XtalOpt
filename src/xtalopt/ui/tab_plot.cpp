/**********************************************************************
  XtalOpt - Tools for advanced crystal optimization

  Copyright (C) 2009-2011 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <xtalopt/ui/tab_plot.h>
#include <xtalopt/ui/xtalopt_plot.h>

#include <xtalopt/structures/xtal.h>
#include <xtalopt/ui/dialog.h>
#include <xtalopt/xtalopt.h>

#include <globalsearch/constants.h>
#include <globalsearch/queuemanager.h>
#include <globalsearch/utilities/fileutils.h>

#include <QReadWriteLock>
#include <QSettings>

#include <qwt_scale_engine.h>
#include <qwt_text.h>

#include <float.h>

using namespace GlobalSearch;

namespace XtalOpt {

TabPlot::TabPlot(GlobalSearch::AbstractDialog* parent, XtalOpt* p)
  : AbstractTab(parent, p), m_enablePlotUpdate(true),
    m_plot_mutex(new QReadWriteLock(QReadWriteLock::Recursive))
{
  ui.setupUi(m_tab_widget);

  // Change the margins on the plot for autoscaling
  ui.plot->axisScaleEngine(QwtPlot::yLeft)->setMargins(0.1, 0.1);
  ui.plot->axisScaleEngine(QwtPlot::xBottom)->setMargins(1.0, 1.0);

  // dialog connections
  connect(m_dialog, SIGNAL(moleculeChanged(GlobalSearch::Structure*)), this,
          SLOT(highlightXtal(GlobalSearch::Structure*)));

  // Plot connections
  connect(ui.push_refresh, SIGNAL(clicked()), this, SLOT(refreshPlot()));
  connect(ui.combo_xAxis, SIGNAL(currentIndexChanged(int)), this,
          SLOT(refreshPlot()));
  connect(ui.combo_yAxis, SIGNAL(currentIndexChanged(int)), this,
          SLOT(refreshPlot()));
  connect(ui.cb_labelPoints, SIGNAL(toggled(bool)), this, SLOT(updatePlot()));
  connect(ui.combo_labelType, SIGNAL(currentIndexChanged(int)), this,
          SLOT(updatePlot()));
  connect(ui.cb_showSimilarities, SIGNAL(toggled(bool)), this,
          SLOT(updatePlot()));
  connect(ui.cb_showIncompletes, SIGNAL(toggled(bool)), this,
          SLOT(updatePlot()));
  connect(ui.plot, SIGNAL(selectedMarkerChanged(QwtPlotMarker*)), this,
          SLOT(selectXtal(QwtPlotMarker*)));
  connect(m_search, SIGNAL(refreshAllStructureInfo()), this, SLOT(updatePlot()));
  connect(m_search->queue(), SIGNAL(structureUpdated(GlobalSearch::Structure*)),
          this, SLOT(updatePlot()));
  connect(m_search->tracker(), SIGNAL(newStructureAdded(GlobalSearch::Structure*)),
          this, SLOT(updatePlot()));

  // Enable/disable updating the plot?
  connect(m_search, SIGNAL(disablePlotUpdate()), this, SLOT(disablePlotUpdate()));
  connect(m_search, SIGNAL(enablePlotUpdate()), this, SLOT(enablePlotUpdate()));
  connect(m_search, SIGNAL(updatePlot()), this, SLOT(updatePlot()));

  initialize();
}

TabPlot::~TabPlot()
{
  delete m_plot_mutex;
}

void TabPlot::writeSettings(const QString& filename)
{
  SETTINGS(filename);
  const int version = 4;
  settings->beginGroup("xtalopt/plot/");
  settings->setValue("version", version);
  settings->setValue("x_label", ui.combo_xAxis->currentIndex());
  settings->setValue("y_label", ui.combo_yAxis->currentIndex());
  settings->setValue("showSimilarities", ui.cb_showSimilarities->isChecked());
  settings->setValue("showIncompletes", ui.cb_showIncompletes->isChecked());
  settings->setValue("labelPoints", ui.cb_labelPoints->isChecked());
  settings->setValue("labelType", ui.combo_labelType->currentIndex());
  settings->endGroup();
}

void TabPlot::readSettings(const QString& filename)
{
  SETTINGS(filename);
  settings->beginGroup("xtalopt/plot/");
  int loadedVersion = settings->value("version", 0).toInt();
  ui.combo_xAxis->setCurrentIndex(
    settings->value("x_label", StructureINDX_T).toInt());
  ui.combo_yAxis->setCurrentIndex(
    settings->value("y_label", AboveHull_per_Atm_T).toInt());
  ui.cb_showSimilarities->setChecked(
    settings->value("showSimilarities", false).toBool());
  ui.cb_showIncompletes->setChecked(
    settings->value("showIncompletes", false).toBool());
  ui.cb_labelPoints->setChecked(settings->value("labelPoints", false).toBool());
  ui.combo_labelType->setCurrentIndex(
    settings->value("labelType", Symbol_L).toInt());
  settings->endGroup();

  // Set the default values of x, y, and label menus
  ui.combo_labelType->setCurrentIndex(0);
  ui.combo_yAxis->setCurrentIndex(1);
  ui.combo_xAxis->setCurrentIndex(0);
  ui.cb_labelPoints->setChecked(true);
}

void TabPlot::updateGUI()
{
}

void TabPlot::disconnectGUI()
{
  ui.push_refresh->disconnect();
  ui.combo_xAxis->disconnect();
  ui.combo_yAxis->disconnect();
  ui.cb_labelPoints->disconnect();
  ui.combo_labelType->disconnect();
  ui.cb_showSimilarities->disconnect();
  ui.cb_showIncompletes->disconnect();
  this->disconnect();
  disconnect(m_dialog, 0, this, 0);
  disconnect(m_search, 0, this, 0);
}

void TabPlot::refreshPlot()
{
  // Reset axis scales and then update
  ui.plot->setAxisAutoScale(QwtPlot::yLeft);
  ui.plot->setAxisAutoScale(QwtPlot::xBottom);
  updatePlot();
}

void TabPlot::updatePlot()
{
  // If we have disabled plot updating, just return.
  if (!m_enablePlotUpdate)
    return;

  updateGUI();
  if (!m_search)
    return;

  // Make sure we have structures!
  if (m_search->tracker()->size() == 0)
    return;

  // Lock plot mutex
  m_plot_mutex->lockForWrite();

  // Here, we want to make sure that objectives are shown only if they are present
  // To show the proper number of them, we set the starting point to Objectivei_*
  int numaxisitems = m_search->getObjectivesNum() + Objectivei_T;
  int numsymbitems = m_search->getObjectivesNum() + Objectivei_L;
  ui.combo_xAxis->setMaxCount(numaxisitems);
  ui.combo_yAxis->setMaxCount(numaxisitems);
  ui.combo_labelType->setMaxCount(numsymbitems);
  for (int i = 0; i < m_search->getObjectivesNum(); i++) {
      ui.combo_xAxis->addItem(tr("Objective%1").arg(i+1));
      ui.combo_yAxis->addItem(tr("Objective%1").arg(i+1));
      ui.combo_labelType->addItem(tr("Objective%1").arg(i+1));
  }

  plotTrends();

  m_plot_mutex->unlock();
}

void TabPlot::plotTrends()
{
  // Don't replot while we are updating
  ui.plot->setAutoReplot(false);

  // Store the selected xtal
  Xtal* selectedXtal = m_marker_xtal_map[ui.plot->selectedMarker()];

  // There may be a faster way to do this that doesn't include deleting
  // and making the whole plot over again, but for now, this works. We
  // just delete the whole plot and make it over again...
  m_marker_xtal_map.clear();
  ui.plot->clearAll();

  double x, y;
  int ind;
  Xtal* xtal = nullptr;
  double minE = DBL_MAX;
  size_t lastTraceStructure = -1;
  bool performTrace = false;
  // Load config settings:
  bool labelPoints = ui.cb_labelPoints->isChecked();
  bool showSimilarities = ui.cb_showSimilarities->isChecked();
  bool showIncompletes = ui.cb_showIncompletes->isChecked();
  LabelTypes labelType = LabelTypes(ui.combo_labelType->currentIndex());
  PlotAxes xAxis = PlotAxes(ui.combo_xAxis->currentIndex());
  PlotAxes yAxis = PlotAxes(ui.combo_yAxis->currentIndex());

  if (xAxis == StructureINDX_T && (yAxis == Energy_T || yAxis == Enthalpy_T ||
                               yAxis == Enthalpy_per_Atm_T || yAxis == AboveHull_per_Atm_T)) {
    performTrace = true;
  }

  QReadLocker trackerLocker(m_search->tracker()->rwLock());
  const QList<Structure*> structures(*m_search->tracker()->list());
  for (int i = 0; i < structures.size(); i++) {
    x = y = 0;
    xtal = qobject_cast<Xtal*>(structures[i]);
    QReadLocker xtalLocker(&xtal->lock());
    // Don't plot removed structures or those who have not completed their
    // first INCAR.
    if (!showSimilarities && xtal->getStatus() == Xtal::Similar) {
      continue;
    }

    if (xtal->getStatus() == Xtal::Killed ||
        xtal->getStatus() == Xtal::Removed ||
        xtal->getStatus() == Xtal::ObjectiveFail ||
        xtal->getStatus() == Xtal::ObjectiveDismiss ||
        fabs(xtal->getEnthalpy()) <= ZERO0) {
      continue;
    }

    // If it is anything other than a similar or optimized
    // and showIncompletes is false, don't show it
    if (!showIncompletes && xtal->getStatus() != Xtal::Similar &&
        xtal->getStatus() != Xtal::Optimized) {
      continue;
    }

    bool usePoint = true;
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
        case StructureINDX_T:
          switch (j) {
            case 0:
              x = xtal->getIndex();
              break;
            default:
              y = xtal->getIndex();
              break;
          }
          break;
        case Generation_T:
          switch (j) {
            case 0:
              x = xtal->getGeneration();
              break;
            default:
              y = xtal->getGeneration();
              break;
          }
          break;
        case Enthalpy_T:
          // Skip xtals that don't have enthalpy/energy set
          if (xtal->getEnergy() == 0.0 && !xtal->hasEnthalpy()) {
            usePoint = false;
            continue;
          }
          switch (j) {
            case 0:
              x = xtal->getEnthalpy();
              break;
            default:
              y = xtal->getEnthalpy();
              break;
          }
          break;
        case ParetoFront_T:
          // Skip xtals that don't have pareto front
          if (xtal->getParetoFront() < 0) {
            usePoint = false;
            continue;
          }
          switch (j) {
            case 0:
              x = xtal->getParetoFront();
              break;
            default:
              y = xtal->getParetoFront();
              break;
          }
          break;
        case AboveHull_per_Atm_T:
          // Skip xtals that don't have above hull
          if (std::isnan(xtal->getDistAboveHull())) {
            usePoint = false;
            continue;
          }
          switch (j) {
            case 0:
              x = xtal->getDistAboveHull();
              break;
            default:
              y = xtal->getDistAboveHull();
              break;
          }
          break;
        case Enthalpy_per_Atm_T:
          // Skip xtals that don't have enthalpy/energy set
          if (xtal->getEnergy() == 0.0 && !xtal->hasEnthalpy()) {
            usePoint = false;
            continue;
          }
          switch (j) {
            case 0:
              x = xtal->getEnthalpyPerAtom();
              break;
            default:
              y = xtal->getEnthalpyPerAtom();
              break;
          }
          break;
        case Energy_T:
          // Skip xtals that don't have energy set
          if (xtal->getEnergy() == 0.0) {
            usePoint = false;
            continue;
          }
          switch (j) {
            case 0:
              x = xtal->getEnergy();
              break;
            default:
              y = xtal->getEnergy();
              break;
          }
          break;
        case PV_T:
          // Skip xtals that don't have enthalpy/energy set
          if (xtal->getEnergy() == 0.0 && !xtal->hasEnthalpy()) {
            usePoint = false;
            continue;
          }
          switch (j) {
            case 0:
              x = xtal->getPV();
              break;
            default:
              y = xtal->getPV();
              break;
          }
          break;
        case A_T:
          switch (j) {
            case 0:
              x = xtal->getA();
              break;
            default:
              y = xtal->getA();
              break;
          }
          break;
        case B_T:
          switch (j) {
            case 0:
              x = xtal->getB();
              break;
            default:
              y = xtal->getB();
              break;
          }
          break;
        case C_T:
          switch (j) {
            case 0:
              x = xtal->getC();
              break;
            default:
              y = xtal->getC();
              break;
          }
          break;
        case Alpha_T:
          switch (j) {
            case 0:
              x = xtal->getAlpha();
              break;
            default:
              y = xtal->getAlpha();
              break;
          }
          break;
        case Beta_T:
          switch (j) {
            case 0:
              x = xtal->getBeta();
              break;
            default:
              y = xtal->getBeta();
              break;
          }
          break;
        case Gamma_A:
          switch (j) {
            case 0:
              x = xtal->getGamma();
              break;
            default:
              y = xtal->getGamma();
              break;
          }
          break;
        case Volume_T:
          switch (j) {
            case 0:
              x = xtal->getVolume();
              break;
            default:
              y = xtal->getVolume();
              break;
          }
          break;
        case Volume_per_Atm_T:
          switch (j) {
            case 0:
              x = xtal->getVolumePerAtom();
              break;
            default:
              y = xtal->getVolumePerAtom();
              break;
          }
          break;
        default:
          // Objectives in multi-objective run. Since there is no fixed number of
          //   objectives; and MSVC does not support "case range", we put them
          //   under "default".
          // Their index in the list of options starts from Objectivei_T,
          //   so the proper index for objective value array is "ind - Objectivei_T"
          if (ind >= Objectivei_T) {
            // Skip xtals that don't have objectives calculated
            if (xtal->getStrucObjState() == Structure::Os_NotCalculated) {
              usePoint = false;
              continue;
            }
            switch (j) {
              case 0:
                x = xtal->getStrucObjValues(ind - Objectivei_T);
                break;
              default:
                y = xtal->getStrucObjValues(ind - Objectivei_T);
                break;
            }
          }
      }
    }

    if (!usePoint)
      continue;

    QwtPlotMarker* pm = addXtalToPlot(xtal, x, y);

    // See if we should draw another line for the trace
    // This trace assumes the xtals will be ordered based on structure index
    if (performTrace) {
      if (lastTraceStructure == -1) {
        lastTraceStructure = xtal->getIndex();
        minE = y;
      } else if (y < minE) {
        plotTrace(lastTraceStructure, minE, xtal->getIndex(), y);
        lastTraceStructure = xtal->getIndex();
        minE = y;
      } else {
        ui.plot->addHorizontalPlotLine(lastTraceStructure, xtal->getIndex() , minE);
      }
    }

    // Set point label if requested
    QString s;
    if (labelPoints) {
      switch (labelType) {
        case Number_L:
          s = QString::number(xtal->getSpaceGroupNumber());
          break;
        case Symbol_L:
          s = xtal->getSpaceGroupSymbol();
          break;
        case Enthalpy_L:
          s = QString::number(xtal->getEnthalpy(), 'g', 5);
          break;
        case Enthalpy_per_Atm_L:
          s = QString::number(xtal->getEnthalpyPerAtom(), 'g', 5);
          break;
        case AboveHull_per_Atm_L:
          s = QString::number(xtal->getDistAboveHull(), 'g', 5);
          break;
        case ParetoFront_L:
          s = QString::number(xtal->getParetoFront());
          break;
        case Energy_L:
          s = QString::number(xtal->getEnergy(), 'g', 5);
          break;
        case PV_L:
          s = QString::number(xtal->getPV(), 'g', 5);
          break;
        case Volume_per_Atm_L:
          s = QString::number(xtal->getVolumePerAtom(), 'g', 5);
          break;
        case Generation_L:
          s = QString::number(xtal->getGeneration());
          break;
        case StructureINDX_L:
          s = QString::number(xtal->getIndex());
          break;
        case StructureTAG_L:
          s = xtal->getTag();
          break;
        default:
          // Objectives in multi-objective run. Since there is no fixed number of
          //   objectives; and MSVC does not support "case range", we put them
          //   under "default".
          // Their index in the list of options starts from Objectivei_L,
          //   so the proper index for objective value array is "labelTyep - Objectivei_L"
          if (labelType >= Objectivei_L) {
            s = QString::number(xtal->getStrucObjValues(labelType - Objectivei_L));
          } else {
            s = xtal->getSpaceGroupSymbol();
          }
        }
      QwtText text(s);
      text.setColor(Qt::black);
      pm->setLabel(text);
      pm->setLabelAlignment(Qt::AlignBottom);
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
      case StructureINDX_T:
        label = tr("Structure Index");
        break;
      case Generation_T:
        label = tr("Generation");
        break;
      case Enthalpy_T:
        label = tr("Enthalpy");
        break;
      case AboveHull_per_Atm_T:
        label = tr("Above Hull per Atom");
        break;
      case ParetoFront_T:
        label = tr("Pareto Front");
        break;
      case Enthalpy_per_Atm_T: // PSA Enthalpy per atom
        label = tr("Enthalpy per Atom");
        break;
      case Energy_T:
        label = tr("Energy");
        break;
      case PV_T:
        label = tr("Enthalpy PV Term");
        break;
      case A_T:
        label = tr("A");
        break;
      case B_T:
        label = tr("B");
        break;
      case C_T:
        label = tr("C");
        break;
      case Alpha_T:
        label = "<HTML>&alpha;</HTML>";
        break;
      case Beta_T:
        label = "<HTML>&beta;</HTML>";
        break;
      case Gamma_A:
        label = "<HTML>&gamma;</HTML>";
        break;
      case Volume_T:
        label = tr("Volume");
        break;
      case Volume_per_Atm_T:
        label = tr("Volume per Atom");
        break;
      default:
        // Objectives in multi-objective run. Since there is no fixed number of
        //   objectives; and MSVC does not support "case range", we put them
        //   under "default".
        // Their index in the list of options starts from Objectivei_T,
        //   but their "shown" index starts from 1; so we have "ind - Objectivei_T + 1"
        if (ind >= Objectivei_T) {
          label = tr("Objective%1").arg(ind - Objectivei_T + 1);
        }
      }
    if (j == 0)
      ui.plot->setXTitle(label);
    else
      ui.plot->setYTitle(label);
  }

  // If the selected xtal still exists, select that one. This function
  // doesn't do anything if nullptr is passed to it.
  ui.plot->selectMarker(m_marker_xtal_map.key(selectedXtal));

  ui.plot->setAutoReplot(true);
  ui.plot->replot();
}

void TabPlot::plotDistHist()
{
  /*
      // Initialize vars
      m_plotObject = new PlotObject(Qt::red, PlotObject::Bars);
      double x, y;
      PlotPoint *pp;
      QList<double> d, f, f_temp;

      // Determine xtal
      int ind = ui.combo_distHistXtal->currentIndex();
      if (ind < 0 || ind > m_search->tracker()->size() - 1) {
        ind = 0;
      }
      Xtal* xtal = qobject_cast<Xtal*>(m_search->tracker()->at(ind));

      // Get histogram
      // If no atoms selected...
      xtal->lock().lockForRead();
      xtal->getIADHistogram(&d, &f, 0, 15, .1);
      xtal->lock().unlock();

      // Selected atom histogram section removed due to removal of
      // dependence on Avogadro. A new strategy will need to be used
      // to add it back in in the future (in case someone wants it)

      // Populate plot object
      for (int i = 0; i < d.size(); i++) {
        x = d.at(i);
        y = f.at(i);
        pp = m_plotObject->addPoint(x,y);
      }

      ui.plot_plot->axis(PlotWidget::BottomAxis)->setLabel(tr("Distance"));
      ui.plot_plot->axis(PlotWidget::LeftAxis)->setLabel(tr("Count"));

      ui.plot_plot->addPlotObject(m_plotObject);

      // Set default limits
      ui.plot_plot->scaleLimits();
      ui.plot_plot->setDefaultLimits(0, 8, 0,
     ui.plot_plot->dataRect().bottom());
  */
}

QwtPlotMarker* TabPlot::addXtalToPlot(Xtal* xtal, double x, double y)
{
  QwtPlotMarker* pm = nullptr;
  if (xtal->getStatus() == Xtal::Similar) {
    // Dark Green Square
    pm = ui.plot->addPlotPoint(x, y, QwtSymbol::Rect, QBrush(Qt::darkGreen),
                               QPen(Qt::darkGreen));
  } else if (xtal->skippedOptimization()) {
    // Blue violet
    QColor blueViolet(138, 43, 226, 255);
    pm = ui.plot->addPlotPoint(x, y, QwtSymbol::Triangle, QBrush(blueViolet),
                               QPen(QBrush(Qt::black), 2));
  } else {
    // Blue Triangle
    pm = ui.plot->addPlotPoint(x, y, QwtSymbol::Triangle, QBrush(Qt::blue),
                               QPen(Qt::blue));
  }

  m_marker_xtal_map[pm] = xtal;
  return pm;
}

void TabPlot::plotTrace(double x1, double y1, double x2, double y2)
{
  ui.plot->addHorizontalPlotLine(x1, x2, y1);
  ui.plot->addVerticalPlotLine(x2, y1, y2);
}

void TabPlot::selectXtal(QwtPlotMarker* pm)
{
  Xtal* xtal = m_marker_xtal_map[pm];
  if (!xtal)
    return;

  emit moleculeChanged(xtal);
}

void TabPlot::selectMoleculeFromIndex(int index)
{
  if (index < 0 || index > m_search->tracker()->size() - 1) {
    index = 0;
  }
  if (m_search->tracker()->size() == 0) {
    return;
  }
  emit moleculeChanged(m_search->tracker()->at(index));
}

void TabPlot::highlightXtal(Structure* s)
{
  Xtal* xtal = qobject_cast<Xtal*>(s);

  ui.plot->selectMarker(m_marker_xtal_map.key(xtal));
}
}
