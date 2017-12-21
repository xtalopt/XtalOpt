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

#include <globalsearch/queuemanager.h>
#include <globalsearch/utilities/fileutils.h>

#include <QReadWriteLock>
#include <QSettings>

#include <qwt_scale_engine.h>

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

  // We will have a default value of 1 in this list
  m_formulaUnitsList.append(1);

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
  connect(ui.cb_showDuplicates, SIGNAL(toggled(bool)), this,
          SLOT(updatePlot()));
  connect(ui.cb_showIncompletes, SIGNAL(toggled(bool)), this,
          SLOT(updatePlot()));
  connect(ui.cb_showSpecifiedFU, SIGNAL(toggled(bool)), this,
          SLOT(updatePlot()));
  connect(ui.edit_showSpecifiedFU, SIGNAL(editingFinished()), this,
          SLOT(updatePlotFormulaUnits()));
  connect(ui.plot, SIGNAL(selectedMarkerChanged(QwtPlotMarker*)), this,
          SLOT(selectXtal(QwtPlotMarker*)));
  connect(m_opt, SIGNAL(refreshAllStructureInfo()), this, SLOT(updatePlot()));
  connect(m_opt->queue(), SIGNAL(structureUpdated(GlobalSearch::Structure*)),
          this, SLOT(updatePlot()));
  connect(m_opt->tracker(), SIGNAL(newStructureAdded(GlobalSearch::Structure*)),
          this, SLOT(updatePlot()));

  // Enable/disable updating the plot?
  connect(m_opt, SIGNAL(disablePlotUpdate()), this, SLOT(disablePlotUpdate()));
  connect(m_opt, SIGNAL(enablePlotUpdate()), this, SLOT(enablePlotUpdate()));
  connect(m_opt, SIGNAL(updatePlot()), this, SLOT(updatePlot()));

  initialize();
}

TabPlot::~TabPlot()
{
  delete m_plot_mutex;
}

void TabPlot::writeSettings(const QString& filename)
{
  SETTINGS(filename);
  const int version = 1;
  settings->beginGroup("xtalopt/plot/");
  settings->setValue("version", version);
  settings->setValue("x_label", ui.combo_xAxis->currentIndex());
  settings->setValue("y_label", ui.combo_yAxis->currentIndex());
  settings->setValue("showDuplicates", ui.cb_showDuplicates->isChecked());
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
    settings->value("x_label", Structure_T).toInt());
  ui.combo_yAxis->setCurrentIndex(
    settings->value("y_label", Enthalpy_per_FU_T).toInt());
  ui.cb_showDuplicates->setChecked(
    settings->value("showDuplicates", false).toBool());
  ui.cb_showIncompletes->setChecked(
    settings->value("showIncompletes", false).toBool());
  ui.cb_labelPoints->setChecked(settings->value("labelPoints", false).toBool());
  ui.combo_labelType->setCurrentIndex(
    settings->value("labelType", Symbol_L).toInt());
  settings->endGroup();
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
  ui.cb_showDuplicates->disconnect();
  ui.cb_showIncompletes->disconnect();
  this->disconnect();
  disconnect(m_dialog, 0, this, 0);
  disconnect(m_opt, 0, this, 0);
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
  if (!m_opt)
    return;

  // Make sure we have structures!
  if (m_opt->tracker()->size() == 0)
    return;

  // Lock plot mutex
  m_plot_mutex->lockForWrite();

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
  size_t lastTraceStructure = 0;
  bool performTrace = false;
  // Load config settings:
  bool labelPoints = ui.cb_labelPoints->isChecked();
  bool showDuplicates = ui.cb_showDuplicates->isChecked();
  bool showIncompletes = ui.cb_showIncompletes->isChecked();
  bool showSpecifiedFU = ui.cb_showSpecifiedFU->isChecked();
  LabelTypes labelType = LabelTypes(ui.combo_labelType->currentIndex());
  PlotAxes xAxis = PlotAxes(ui.combo_xAxis->currentIndex());
  PlotAxes yAxis = PlotAxes(ui.combo_yAxis->currentIndex());

  if (xAxis == Structure_T && (yAxis == Energy_T || yAxis == Enthalpy_T ||
                               yAxis == Enthalpy_per_FU_T)) {
    performTrace = true;
  }

  QReadLocker trackerLocker(m_opt->tracker()->rwLock());
  const QList<Structure*> structures(*m_opt->tracker()->list());
  for (int i = 0; i < structures.size(); i++) {
    x = y = 0;
    xtal = qobject_cast<Xtal*>(structures[i]);
    QReadLocker xtalLocker(&xtal->lock());
    // Don't plot removed structures or those who have not completed their
    // first INCAR. Also only plot specified formula units if box is checked.
    if (showSpecifiedFU) {
      bool inTheList = false;
      for (int i = 0; i < m_formulaUnitsList.size(); i++) {
        if (m_formulaUnitsList.at(i) == xtal->getFormulaUnits()) {
          inTheList = true;
        }
      }
      if (inTheList == false) {
        continue;
      }
    }

    if (!showDuplicates && (xtal->getStatus() == Xtal::Duplicate ||
                            xtal->getStatus() == Xtal::Supercell)) {
      continue;
    }

    if (xtal->getStatus() == Xtal::Killed ||
        xtal->getStatus() == Xtal::Removed ||
        fabs(xtal->getEnthalpy()) <= 1e-50) {
      continue;
    }

    // If it is anything other than a duplicate, supercell, or optimized
    // and showIncompletes is false, don't show it
    if (!showIncompletes && xtal->getStatus() != Xtal::Duplicate &&
        xtal->getStatus() != Xtal::Supercell &&
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
        case Enthalpy_per_FU_T:
          // Skip xtals that don't have enthalpy/energy set
          if (xtal->getEnergy() == 0.0 && !xtal->hasEnthalpy()) {
            usePoint = false;
            continue;
          }
          switch (j) {
            case 0:
              x = xtal->getEnthalpy() /
                  static_cast<double>(xtal->getFormulaUnits());
              break;
            default:
              y = xtal->getEnthalpy() /
                  static_cast<double>(xtal->getFormulaUnits());
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
        case Hardness_T:
          // Skip xtals that don't have a hardness set
          if (xtal->vickersHardness() < 0.0) {
            usePoint = false;
            continue;
          }
          switch (j) {
            case 0:
              x = xtal->vickersHardness();
              break;
            default:
              y = xtal->vickersHardness();
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
        case Volume_per_FU_T:
          switch (j) {
            case 0:
              x = xtal->getVolume() /
                  static_cast<double>(xtal->getFormulaUnits());
              break;
            default:
              y = xtal->getVolume() /
                  static_cast<double>(xtal->getFormulaUnits());
              break;
          }
          break;
        case Formula_Units_T:
          switch (j) {
            case 0:
              x = xtal->getFormulaUnits();
              break;
            default:
              y = xtal->getFormulaUnits();
              break;
          }
          break;
      }
    }

    if (!usePoint)
      continue;

    QwtPlotMarker* pm = addXtalToPlot(xtal, x, y);

    // See if we should draw another line for the trace
    // This trace assumes the xtals will be ordered based on structure number
    if (performTrace) {
      if (lastTraceStructure == 0) {
        lastTraceStructure = i + 1;
        minE = y;
      } else if (y < minE) {
        plotTrace(lastTraceStructure, minE, i + 1, y);
        lastTraceStructure = i + 1;
        minE = y;
      } else {
        ui.plot->addHorizontalPlotLine(lastTraceStructure, i + 1, minE);
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
        default:
          s = xtal->getSpaceGroupSymbol();
          break;
        case Enthalpy_L:
          s = QString::number(xtal->getEnthalpy(), 'g', 5);
          break;
        case Enthalpy_Per_FU_L:
          s = QString::number(xtal->getEnthalpy() / xtal->getFormulaUnits(),
                              'g', 5);
          break;
        case Energy_L:
          s = QString::number(xtal->getEnergy(), 'g', 5);
          break;
        case Hardness_L:
          s = QString::number(xtal->vickersHardness(), 'g', 5);
          break;
        case PV_L:
          s = QString::number(xtal->getPV(), 'g', 5);
          break;
        case Volume_L:
          s = QString::number(xtal->getVolume(), 'g', 5);
          break;
        case Generation_L:
          s = QString::number(xtal->getGeneration());
          break;
        case Structure_L:
          s = QString::number(i);
          break;
        case Formula_Units_L:
          s = QString::number(xtal->getFormulaUnits());
          break;
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
      case Structure_T:
        label = tr("Structure");
        break;
      case Generation_T:
        label = tr("Generation");
        break;
      case Enthalpy_T:
        label = tr("Enthalpy (eV)");
        break;
      case Enthalpy_per_FU_T: // PSA Enthalpy per atom
        label = tr("Enthalpy per FU (eV)");
        break;
      case Energy_T:
        label = tr("Energy (eV)");
        break;
      case Hardness_T:
        label = tr("Vickers Hardness (GPa)");
        break;
      case PV_T:
        label = tr("Enthalpy PV term (eV)");
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
      case Volume_per_FU_T:
        label = tr("Volume per FU");
        break;
      case Formula_Units_T:
        label = tr("Formula Units");
        break;
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
      if (ind < 0 || ind > m_opt->tracker()->size() - 1) {
        ind = 0;
      }
      Xtal* xtal = qobject_cast<Xtal*>(m_opt->tracker()->at(ind));

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
  if (xtal->getStatus() == Xtal::Duplicate) {
    // Dark Green Square
    pm = ui.plot->addPlotPoint(x, y, QwtSymbol::Rect, QBrush(Qt::darkGreen),
                               QPen(Qt::darkGreen));
  } else if (xtal->getStatus() == Xtal::Supercell) {
    // Orange Square
    pm = ui.plot->addPlotPoint(x, y, QwtSymbol::Rect, QColor(204, 102, 0, 255),
                               QColor(204, 102, 0, 255));
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
  if (index < 0 || index > m_opt->tracker()->size() - 1) {
    index = 0;
  }
  if (m_opt->tracker()->size() == 0) {
    return;
  }
  emit moleculeChanged(m_opt->tracker()->at(index));
}

void TabPlot::highlightXtal(Structure* s)
{
  Xtal* xtal = qobject_cast<Xtal*>(s);

  ui.plot->selectMarker(m_marker_xtal_map.key(xtal));
}

// Almost identical to the updateFormulaUnits() function in tab_init.cpp.
// It functions in the same way.
void TabPlot::updatePlotFormulaUnits()
{
  m_formulaUnitsList.clear();
  QString tmp;
  QList<unsigned int> tempFormulaUnitsList =
    FileUtils::parseUIntString(ui.edit_showSpecifiedFU->text(), tmp);

  // If nothing valid was entered, return 1
  if (tempFormulaUnitsList.size() == 0) {
    m_formulaUnitsList.append(1);
    tmp = "1";
    ui.edit_showSpecifiedFU->setText(tmp.trimmed());
    return;
  }

  m_formulaUnitsList = tempFormulaUnitsList;

  // Update UI
  ui.edit_showSpecifiedFU->setText(tmp.trimmed());
  refreshPlot();
}
}
