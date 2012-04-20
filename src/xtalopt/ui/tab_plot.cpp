/**********************************************************************
  XtalOpt - Tools for advanced crystal optimization

  Copyright (C) 2009-2011 by David Lonie

  This library is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as
  published by the Free Software Foundation; either version 2.1 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include <xtalopt/ui/tab_plot.h>

#include <xtalopt/xtalopt.h>
#include <xtalopt/ui/dialog.h>

#include <avogadro/glwidget.h>
#include <avogadro/primitive.h>
#include <avogadro/primitivelist.h>

#include <QtCore/QSettings>
#include <QtCore/QReadWriteLock>

#include <float.h>

using namespace GlobalSearch;
using namespace Avogadro;

namespace XtalOpt {

  TabPlot::TabPlot( XtalOptDialog *parent, XtalOpt *p ) :
    AbstractTab(parent, p),
    m_plot_mutex(new QReadWriteLock()),
    m_plotObject(0)
  {
    ui.setupUi(m_tab_widget);

    // Plot setup
    ui.plot_plot->setAntialiasing(true);
    updatePlot();

    // dialog connections
    connect(m_dialog, SIGNAL(moleculeChanged(GlobalSearch::Structure*)),
            this, SLOT(highlightXtal(GlobalSearch::Structure*)));
    connect(m_opt, SIGNAL(sessionStarted()),
            this, SLOT(populateXtalList()));
    connect(m_opt, SIGNAL(readOnlySessionStarted()),
            this, SLOT(populateXtalList()));
    connect(m_dialog->getGLWidget(), SIGNAL(mouseRelease(QMouseEvent*)),
            this, SLOT(updatePlot()));

    // Plot connections
    connect(ui.push_refresh, SIGNAL(clicked()),
            this, SLOT(refreshPlot()));
    connect(ui.combo_xAxis, SIGNAL(currentIndexChanged(int)),
            this, SLOT(refreshPlot()));
    connect(ui.combo_yAxis, SIGNAL(currentIndexChanged(int)),
            this, SLOT(refreshPlot()));
    connect(ui.combo_plotType, SIGNAL(currentIndexChanged(int)),
            this, SLOT(refreshPlot()));
    connect(ui.combo_distHistXtal, SIGNAL(currentIndexChanged(int)),
            this, SLOT(refreshPlot()));
    connect(ui.combo_distHistXtal, SIGNAL(currentIndexChanged(int)),
            this, SLOT(selectMoleculeFromIndex(int)));
    connect(ui.cb_labelPoints, SIGNAL(toggled(bool)),
            this, SLOT(updatePlot()));
    connect(ui.combo_labelType, SIGNAL(currentIndexChanged(int)),
            this, SLOT(updatePlot()));
    connect(ui.cb_showDuplicates, SIGNAL(toggled(bool)),
            this, SLOT(updatePlot()));
    connect(ui.cb_showIncompletes, SIGNAL(toggled(bool)),
            this, SLOT(updatePlot()));
    connect(ui.plot_plot, SIGNAL(pointClicked(double, double)),
            this, SLOT(selectMoleculeFromPlot(double, double)));
    connect(m_opt, SIGNAL(refreshAllStructureInfo()),
            this, SLOT(refreshPlot()));
    connect(m_opt, SIGNAL(refreshAllStructureInfo()),
            this, SLOT(populateXtalList()));
    connect(m_opt->queue(), SIGNAL(structureUpdated(GlobalSearch::Structure*)),
            this, SLOT(populateXtalList()));
    connect(m_opt->tracker(), SIGNAL(newStructureAdded(GlobalSearch::Structure*)),
            this, SLOT(populateXtalList()));
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

  void TabPlot::writeSettings(const QString &filename)
  {
    SETTINGS(filename);
    const int VERSION = 1;
    settings->beginGroup("xtalopt/plot/");
    settings->setValue("version",         VERSION);
    settings->setValue("x_label",         ui.combo_xAxis->currentIndex());
    settings->setValue("y_label",         ui.combo_yAxis->currentIndex());
    settings->setValue("showDuplicates",  ui.cb_showDuplicates->isChecked());
    settings->setValue("showIncompletes", ui.cb_showIncompletes->isChecked());
    settings->setValue("labelPoints",     ui.cb_labelPoints->isChecked());
    settings->setValue("labelType",       ui.combo_labelType->currentIndex());
    settings->setValue("plotType",        ui.combo_plotType->currentIndex());
    settings->endGroup();

    DESTROY_SETTINGS(filename);
  }

  void TabPlot::readSettings(const QString &filename)
  {
    SETTINGS(filename);
    settings->beginGroup("xtalopt/plot/");
    int loadedVersion = settings->value("version", 0).toInt();
    ui.combo_xAxis->setCurrentIndex( settings->value("x_label", Structure_T).toInt());
    ui.combo_yAxis->setCurrentIndex( settings->value("y_label", Enthalpy_T).toInt());
    ui.cb_showDuplicates->setChecked( settings->value("showDuplicates", false).toBool());
    ui.cb_showIncompletes->setChecked( settings->value("showIncompletes", false).toBool());
    ui.cb_labelPoints->setChecked( settings->value("labelPoints", false).toBool());
    ui.combo_labelType->setCurrentIndex( settings->value("labelType", Symbol_L).toInt());
    ui.combo_plotType->setCurrentIndex( settings->value("plotType", Trend_PT).toInt());
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

  void TabPlot::disconnectGUI() {
    ui.push_refresh->disconnect();
    ui.combo_xAxis->disconnect();
    ui.combo_yAxis->disconnect();
    ui.combo_plotType->disconnect();
    ui.combo_distHistXtal->disconnect();
    ui.cb_labelPoints->disconnect();
    ui.combo_labelType->disconnect();
    ui.cb_showDuplicates->disconnect();
    ui.cb_showIncompletes->disconnect();
    ui.plot_plot->disconnect();
    this->disconnect();
    disconnect(m_dialog, 0, this, 0);
    disconnect(m_opt, 0, this, 0);
  }

  void TabPlot::lockClearAndSelectPoint(PlotPoint *pp)
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
      connect(ui.plot_plot, SIGNAL(pointClicked(double, double)),
              this, SLOT(selectMoleculeFromPlot(double, double)));
    }
  }

  void TabPlot::updatePlot()
  {
    updateGUI();
    if (!m_opt) return;

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

    m_plotObject = new PlotObject (Qt::red, PlotObject::Points, 4, PlotObject::Triangle);

    double x, y;
    int ind;
    Xtal* xtal;
    PlotPoint *pp;
    // Load config settings:
    bool labelPoints		= ui.cb_labelPoints->isChecked();
    bool showDuplicates		= ui.cb_showDuplicates->isChecked();
    bool showIncompletes        = ui.cb_showIncompletes->isChecked();
    LabelTypes labelType	= LabelTypes(ui.combo_labelType->currentIndex());
    PlotAxes xAxis		= PlotAxes(ui.combo_xAxis->currentIndex());
    PlotAxes yAxis              = PlotAxes(ui.combo_yAxis->currentIndex());

    // For minimum-energy traces
    double minE = DBL_MAX;
    PlotObject *traceObject = 0;
    unsigned int lastGoodTraceIndex = 0; // Used to trim points from end
    if (xAxis == Structure_T &&
        (yAxis == Energy_T ||
         yAxis == Enthalpy_T)) {
      traceObject = new PlotObject(Qt::gray, PlotObject::Lines, 1);
    }

    const QList<Structure*> structures (*m_opt->tracker()->list());
    for (int i = 0; i < structures.size(); i++) {
      // Always put a trace point in for each structure index
      if (traceObject && minE != DBL_MAX) {
        traceObject->addPoint(i+1, minE);
      }
      x = y = 0;
      xtal = qobject_cast<Xtal*>(structures[i]);
      QReadLocker xtalLocker (xtal->lock());
      // Don't plot removed structures or those who have not completed their first INCAR.
      if ((xtal->getStatus() != Xtal::Optimized && !showIncompletes)) {
        if  (!(xtal->getStatus() == Xtal::Duplicate && showDuplicates)) {
          continue;
        }
      }

      if  (xtal->getStatus() == Xtal::Duplicate && !showDuplicates) {
        continue;
      }

      if (xtal->getStatus() == Xtal::Killed ||
          xtal->getStatus() == Xtal::Removed ||
          fabs(xtal->getEnthalpy()) <= 1e-50) {
        continue;
      }

      // Update trace
      if (traceObject) {
        // Get current value
        double currentE;
        if (yAxis == Energy_T) {
          currentE = xtal->getEnergy();
        }
        else if (yAxis == Enthalpy_T) {
          currentE = xtal->getEnthalpy();
        }

        // Update minimum if needed
        if (minE > currentE) {
          minE = currentE;
        }
        // The two points ensure that the lines between points are
        // correct.
        traceObject->addPoint(i+1, minE);
      }

      // Get X/Y data
      for (int j = 0; j < 2; j++) { // 0 = x, 1 = y
        switch (j) {
        case 0:         ind = xAxis; break;
        default:        ind = yAxis; break;
        }

        switch (ind) {
        case Structure_T:
          switch (j) {
          case 0:       x = i+1; break;
          default:      y = i+1; break;
          }
          break;
        case Generation_T:
          switch (j) {
          case 0:       x = xtal->getGeneration(); break;
          default:      y = xtal->getGeneration(); break;
          }
          break;
        case Enthalpy_T:
          // Skip xtals that don't have enthalpy/energy set
          if (xtal->getEnergy() == 0.0 && !xtal->hasEnthalpy()) continue;
          switch (j) {
          case 0:       x = xtal->getEnthalpy(); break;
          default:      y = xtal->getEnthalpy(); break;
          }
          break;
        case Energy_T:
          // Skip xtals that don't have energy set
          if (xtal->getEnergy() == 0.0) continue;
          switch (j) {
          case 0:       x = xtal->getEnergy(); break;
          default:      y = xtal->getEnergy(); break;
          }
          break;
        case PV_T:
          // Skip xtals that don't have enthalpy/energy set
          if (xtal->getEnergy() == 0.0 && !xtal->hasEnthalpy()) continue;
          switch (j) {
          case 0:       x = xtal->getPV(); break;
          default:      y = xtal->getPV(); break;
          }
          break;
        case A_T:
          switch (j) {
          case 0:       x = xtal->getA(); break;
          default:      y = xtal->getA(); break;
          }
          break;
        case B_T:
          switch (j) {
          case 0:       x = xtal->getB(); break;
          default:      y = xtal->getB(); break;
          }
          break;
        case C_T:
          switch (j) {
          case 0:       x = xtal->getC(); break;
          default:      y = xtal->getC(); break;
          }
          break;
        case Alpha_T:
          switch (j) {
          case 0:       x = xtal->getAlpha(); break;
          default:      y = xtal->getAlpha(); break;
          }
          break;
        case Beta_T:
          switch (j) {
          case 0:       x = xtal->getBeta(); break;
          default:      y = xtal->getBeta(); break;
          }
          break;
        case Gamma_A:
          switch (j) {
          case 0:       x = xtal->getGamma(); break;
          default:      y = xtal->getGamma(); break;
          }
          break;
        case Volume_T:
          switch (j) {
          case 0:       x = xtal->getVolume(); break;
          default:      y = xtal->getVolume(); break;
          }
        }
      }
      if (traceObject) {
        lastGoodTraceIndex = traceObject->points().size() - 1;
      }
      pp = m_plotObject->addPoint(x,y);
      // Store index for later lookup
      pp->setCustomData(i);
      // Set point label if requested
      if (labelPoints) {
        switch (labelType) {
        case Number_L:
          pp->setLabel(QString::number(xtal->getSpaceGroupNumber()));
          break;
        case Symbol_L:
        default:
          pp->setLabel(xtal->getSpaceGroupSymbol());
          break;
        case Enthalpy_L:
          pp->setLabel(QString::number(xtal->getEnthalpy(), 'g', 5));
          break;
        case Energy_L:
          pp->setLabel(QString::number(xtal->getEnergy(), 'g', 5));
          break;
        case PV_L:
          pp->setLabel(QString::number(xtal->getPV(), 'g', 5));
          break;
        case Volume_L:
          pp->setLabel(QString::number(xtal->getVolume(), 'g', 5));
          break;
        case Generation_L:
          pp->setLabel(QString::number(xtal->getGeneration()));
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
      case 0:   ind = ui.combo_xAxis->currentIndex(); break;
      default:  ind = ui.combo_yAxis->currentIndex(); break;
      }

      QString label;
      switch (ind) {
      case Structure_T:
        label = tr("Structure");
        switch (j) {
        case 0:         ui.plot_plot->axis(PlotWidget::BottomAxis)->setLabel(label); break;
        default:        ui.plot_plot->axis(PlotWidget::LeftAxis)->setLabel(label); break;
        }
        break;
      case Generation_T:
        label = tr("Generation");
        switch (j) {
        case 0:         ui.plot_plot->axis(PlotWidget::BottomAxis)->setLabel(label); break;
        default:        ui.plot_plot->axis(PlotWidget::LeftAxis)->setLabel(label); break;
        }
        break;
      case Enthalpy_T:
        label = tr("Enthalpy (eV)");
        switch (j) {
        case 0:         ui.plot_plot->axis(PlotWidget::BottomAxis)->setLabel(label); break;
        default:        ui.plot_plot->axis(PlotWidget::LeftAxis)->setLabel(label); break;
        }
        break;
      case Energy_T:
        label = tr("Energy (eV)");
        switch (j) {
        case 0:         ui.plot_plot->axis(PlotWidget::BottomAxis)->setLabel(label); break;
        default:        ui.plot_plot->axis(PlotWidget::LeftAxis)->setLabel(label); break;
        }
        break;
      case PV_T:
        label = tr("Enthalpy PV term (eV)");
        switch (j) {
        case 0:         ui.plot_plot->axis(PlotWidget::BottomAxis)->setLabel(label); break;
        default:        ui.plot_plot->axis(PlotWidget::LeftAxis)->setLabel(label); break;
        }
        break;
      case A_T:
        label = tr("A");
        switch (j) {
        case 0:         ui.plot_plot->axis(PlotWidget::BottomAxis)->setLabel(label); break;
        default:        ui.plot_plot->axis(PlotWidget::LeftAxis)->setLabel(label); break;
        }
        break;
      case B_T:
        label = tr("B");
        switch (j) {
        case 0:         ui.plot_plot->axis(PlotWidget::BottomAxis)->setLabel(label); break;
        default:        ui.plot_plot->axis(PlotWidget::LeftAxis)->setLabel(label); break;
        }
        break;
      case C_T:
        label = tr("C");
        switch (j) {
        case 0:         ui.plot_plot->axis(PlotWidget::BottomAxis)->setLabel(label); break;
        default:        ui.plot_plot->axis(PlotWidget::LeftAxis)->setLabel(label); break;
        }
        break;
      case Alpha_T:
        label = "<HTML>&alpha;</HTML>";
        switch (j) {
        case 0:         ui.plot_plot->axis(PlotWidget::BottomAxis)->setLabel(label); break;
        default:        ui.plot_plot->axis(PlotWidget::LeftAxis)->setLabel(label); break;
        }
        break;
      case Beta_T:
        label = "<HTML>&beta;</HTML>";
        switch (j) {
        case 0:         ui.plot_plot->axis(PlotWidget::BottomAxis)->setLabel(label); break;
        default:        ui.plot_plot->axis(PlotWidget::LeftAxis)->setLabel(label); break;
        }
        break;
      case Gamma_A:
        label = "<HTML>&gamma;</HTML>";
        switch (j) {
        case 0:         ui.plot_plot->axis(PlotWidget::BottomAxis)->setLabel(label); break;
        default:        ui.plot_plot->axis(PlotWidget::LeftAxis)->setLabel(label); break;
        }
        break;
      case Volume_T:
        label = tr("Volume");
        switch (j) {
        case 0:         ui.plot_plot->axis(PlotWidget::BottomAxis)->setLabel(label); break;
        default:        ui.plot_plot->axis(PlotWidget::LeftAxis)->setLabel(label); break;
        }
        break;
      }
    }

    ui.plot_plot->addPlotObject(m_plotObject);
    if (traceObject) {
      int numPoints = traceObject->points().size();
      for (int i = numPoints - 1;
           (i > lastGoodTraceIndex && i >= 0); --i) {
        traceObject->removePoint(i);
      }
      ui.plot_plot->addPlotObject(traceObject);
    }

    // Do not scale if m_plotObject is empty.
    // If we have one point, set limits to something appropriate:
    if (m_plotObject->points().size() == 1) {
      double x = m_plotObject->points().at(0)->x();
      double y = m_plotObject->points().at(0)->y();
      ui.plot_plot->setDefaultLimits(x-1, x+1, y+1, y-1);
    }
    // For multiple points, let plotwidget handle it.
    else if (m_plotObject->points().size() >= 2) {
      // run scaleLimits to set the default limits, but then set the
      // limits to the previous region
      ui.plot_plot->scaleLimits();
      ui.plot_plot->setLimits(oldDataRect.left(),
                              oldDataRect.right(),
                              oldDataRect.top(), // These are backwards from intuition,
                              oldDataRect.bottom()); // but that's how Qt works...
    }
  }

  void TabPlot::plotDistHist()
  {
    // Clear old data...
    ui.plot_plot->resetPlot();

    // Initialize vars
    m_plotObject = new PlotObject (Qt::red, PlotObject::Bars);
    double x, y;
    PlotPoint *pp;
    QList<double> d, f, f_temp;

    // Determine xtal
    int ind = ui.combo_distHistXtal->currentIndex();
    if (ind < 0 || ind > m_opt->tracker()->size() - 1) {
      ind = 0;
    }
    Xtal* xtal = qobject_cast<Xtal*>(m_opt->tracker()->at(ind));

    // Determine selected atoms, if any
    QList<Primitive*> selected = m_dialog->getGLWidget()->selectedPrimitives().subList(Primitive::AtomType);

    // Get histogram
    // If no atoms selected...
    if (selected.size() == 0) {
      xtal->lock()->lockForRead();
      xtal->getIADHistogram(&d, &f, 0, 15, .1);
      xtal->lock()->unlock();
    }
    // If atoms are selected:
    else {
      xtal->lock()->lockForRead();
      for (int i = 0; i < selected.size(); i++) {
        xtal->getIADHistogram(&d, &f_temp, 0, 15, .1, qobject_cast<Atom*>(selected.at(i)));
        if (f.isEmpty()) {
          f = f_temp;
        }
        else {
          for (int j = 0; j < f.size(); j++) {
            f[j] += f_temp[j];
          }
        }
      }
      xtal->lock()->unlock();
    }

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
    ui.plot_plot->setDefaultLimits(0, 8, 0, ui.plot_plot->dataRect().bottom());
  }

  /// @todo move this to a background thread
  void TabPlot::populateXtalList()
  {
    int ind = ui.combo_distHistXtal->currentIndex();
    ui.combo_distHistXtal->blockSignals(true);
    ui.combo_distHistXtal->clear();

    const QList<Structure*> structures (*m_opt->tracker()->list());
    Xtal *xtal;
    QString s;
    for (int i = 0; i < structures.size(); i++) {
      xtal = qobject_cast<Xtal*>(structures.at(i));
      s.clear();
      // index:
      s.append(QString::number(i) + ": ");
      // generation and xtal ID:
      s.append(xtal->getIDString());
      // disposition
      switch (xtal->getStatus()) {
      case Xtal::Optimized:
        s.append(" (o)");
        break;
      case Xtal::StepOptimized:
      case Xtal::WaitingForOptimization:
      case Xtal::Submitted:
      case Xtal::InProcess:
      case Xtal::Empty:
      case Xtal::Restart:
      case Xtal::Updating:
      case Xtal::Preoptimizing:
        s.append(" (p)");
        break;
      case Xtal::Error:
        s.append(" (e)");
        break;
      case Xtal::Killed:
      case Xtal::Removed:
        s.append(" (k)");
        break;
      case Xtal::Duplicate:
        s.append(" (d)");
        break;
      default: break;
      }
      ui.combo_distHistXtal->addItem(s);
    }
    if (ind == -1) ind = 0;
    ui.combo_distHistXtal->setCurrentIndex(ind);
    ui.combo_distHistXtal->blockSignals(false);
  }

  void TabPlot::selectMoleculeFromPlot(double x, double y)
  {
    PlotPoint* pt = NULL;
    unsigned int distance = UINT_MAX;
    unsigned int cur;
    QPoint clickedPt = ui.plot_plot->mapToWidget(QPointF(x,y)).toPoint();
    int cx = clickedPt.x();
    int cy = clickedPt.y();
    QPoint refPt;
    int dx, dy;
    foreach ( PlotPoint *pp, m_plotObject->points() ) {
      refPt = ui.plot_plot->mapToWidget(pp->position()).toPoint();
      dx = refPt.x() - cx;
      dy = refPt.y() - cy;
      // Squared distance. Don't bother with sqrts here:
      cur = dx*dx + dy*dy;
      if ( cur < distance ) {
        pt = pp;
        distance = cur;
      }
    }
    selectMoleculeFromPlot(pt);
  }

  void TabPlot::selectMoleculeFromPlot(PlotPoint *pp)
  {
    if (!pp) return;
    int index = pp->customData().toInt();
    selectMoleculeFromIndex(index);
    lockClearAndSelectPoint(pp);
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

  void TabPlot::highlightXtal(Structure *s)
  {
    Xtal *xtal = qobject_cast<Xtal*>(s);

    // Bail out if there is no plotobject in memory
    if (!m_plotObject)
      return;
    QReadLocker plotLocker (m_plot_mutex);
    xtal->lock()->lockForRead();
    uint gen = xtal->getGeneration();
    uint id  = xtal->getIDNumber();
    xtal->lock()->unlock();
    int ind;
    Xtal *txtal;
    for (int i = 0; i < m_opt->tracker()->size(); i++) {
      txtal = qobject_cast<Xtal*>(m_opt->tracker()->at(i));
      txtal->lock()->lockForRead();
      uint tgen = txtal->getGeneration();
      uint tid = txtal->getIDNumber();
      txtal->lock()->unlock();
      if ( tgen == gen &&
           tid == id ) {
        ind = i;
        break;
      }
    }

    if (ui.combo_plotType->currentIndex() == Trend_PT) {
      PlotPoint* pp;
      bool found = false;
      QList<PlotPoint*> *points = new QList<PlotPoint*>(m_plotObject->points());
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
    ui.combo_distHistXtal->blockSignals(true);
    ui.combo_distHistXtal->setCurrentIndex(ind);
    ui.combo_distHistXtal->blockSignals(false);
    if (ui.combo_plotType->currentIndex() == DistHist_PT) {
      refreshPlot();
    }
  }
}

