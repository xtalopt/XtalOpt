/**********************************************************************
  RandomDock -- A tool for analysing a matrix-substrate docking problem

  Copyright (C) 2009 by David Lonie

  This library is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as
  published by the Free Software Foundation; either version 2.1 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include "tab_plot.h"

#include "randomdock.h"
#include "randomdockdialog.h"

#include <avogadro/molecule.h>

#include <QSettings>

using namespace std;

namespace Avogadro {

  TabPlot::TabPlot( RandomDockParams *p ) :
    QObject( p->dialog ), m_params(p)
  {
    qDebug() << "TabPlot::TabPlot( " << p <<  " ) called.";

    m_tab_widget = new QWidget;
    ui.setupUi(m_tab_widget);

    m_plotObject = new PlotObject();

    // Plot setup
    ui.plot_plot->setAntialiasing(true);
    updatePlot();

    // Plot connections
    connect(ui.push_refresh, SIGNAL(clicked()),
            this, SLOT(updatePlot()));
    connect(ui.combo_xAxis, SIGNAL(currentIndexChanged(int)),
            this, SLOT(updatePlot()));
    connect(ui.combo_yAxis, SIGNAL(currentIndexChanged(int)),
            this, SLOT(updatePlot()));
    connect(ui.plot_plot, SIGNAL(pointClicked(PlotPoint*)),
            this, SLOT(selectSceneFromPlot(PlotPoint*)));
    connect(ui.plot_plot, SIGNAL(pointClicked(PlotPoint*)),
            ui.plot_plot, SLOT(clearAndSelectPoint(PlotPoint*)));

    // dialog connections
    connect(p->dialog, SIGNAL(tabsReadSettings()),
            this, SLOT(readSettings()));
    connect(p->dialog, SIGNAL(tabsWriteSettings()),
            this, SLOT(writeSettings()));
    connect(this, SIGNAL(moleculeChanged(Molecule*)),
            p->dialog, SIGNAL(moleculeChanged(Molecule*)));
    connect(p->dialog, SIGNAL(moleculeChanged(Molecule*)),
            this, SLOT(highlightMolecule(Molecule*)));
  }

  TabPlot::~TabPlot()
  {
    qDebug() << "TabPlot::~TabPlot() called";
    // Nothing to do!
  }

  void TabPlot::writeSettings() {
    qDebug() << "TabPlot::writeSettings() called";
    QSettings settings; // Already set up in avogadro/src/main.cpp
    // Nothing to do!
  }

  void TabPlot::readSettings() {
    qDebug() << "TabPlot::readSettings() called";
    QSettings settings; // Already set up in avogadro/src/main.cpp
    // Nothing to do!
  }

  void TabPlot::updatePlot() {
    qDebug() << "TabPlot::updatePlot() called";
    if (!m_params->getScenes()) return;

    // Clear old data...
    ui.plot_plot->resetPlot();

    QReadLocker locker (m_params->rwLock);

    // Make sure we have structures!
    if (m_params->getScenes()->size() == 0) return;

    m_plotObject = new PlotObject (Qt::red, PlotObject::Points, 4, PlotObject::Triangle);

    double x, y;
    int ind;
    Scene* scene;
    PlotPoint *pp;
    for (int i = 0; i < m_params->getScenes()->size(); i++) {
      x = y = 0;
      scene = m_params->getSceneAt(i);
      // Don't plot scenes that have not optimized
      if (!scene->isOptimized()) {
        qDebug() << "Skipping unoptimized scene...";
        continue;
      }
      // Get X/Y data
      for (int j = 0; j < 2; j++) { // 0 = x, 1 = y
        switch (j) {
        case 0: 	ind = ui.combo_xAxis->currentIndex(); break;
        default: 	ind = ui.combo_yAxis->currentIndex(); break;
        }

        switch (ind) {
        case Structure_T:
          switch (j) {
          case 0: 	x = i+1; break;
          default: 	y = i+1; break;
          }
          break;
        case Energy_T:
          switch (j) {
          case 0: 	x = scene->getEnergy(); break;
          default: 	y = scene->getEnergy(); break;
          }
          break;
        }
      }
      pp = m_plotObject->addPoint(x,y);
      pp->setCustomData(i); // Store index
    }

    // Set labels
    for (int j = 0; j < 2; j++) { // 0 = x, 1 = y
      switch (j) {
      case 0: 	ind = ui.combo_xAxis->currentIndex(); break;
      default: 	ind = ui.combo_yAxis->currentIndex(); break;
      }

      QString label;
      switch (ind) {
      case Structure_T:
        label = tr("Structure");
        switch (j) {
        case 0: 	ui.plot_plot->axis(PlotWidget::BottomAxis)->setLabel(label); break;
        default: 	ui.plot_plot->axis(PlotWidget::LeftAxis)->setLabel(label); break;
        }
        break;
      case Energy_T:
        label = tr("Energy (kcal)");
        switch (j) {
        case 0: 	ui.plot_plot->axis(PlotWidget::BottomAxis)->setLabel(label); break;
        default: 	ui.plot_plot->axis(PlotWidget::LeftAxis)->setLabel(label); break;
        }
        break;
      }
    }
    ui.plot_plot->addPlotObject(m_plotObject);
    ui.plot_plot->scaleLimits();
  }

  void TabPlot::selectSceneFromPlot(PlotPoint *pp) {
    qDebug() << "TabPlot::selectSceneFromPlot( " << pp << " ) called";
    if (!pp) return;
    int index = pp->customData().toInt();
    emit moleculeChanged(m_params->getSceneAt(index));
  }

  void TabPlot::highlightMolecule(Molecule *mol) {
    qDebug() << "TabPlot::highlightMolecule( " << mol << " ) called.";
    m_params->rwLock->lockForRead();
    int ind;
    for (int i = 0; i < m_params->getScenes()->size(); i++) {
      if ( m_params->getSceneAt(i) == mol ) {
        ind = i;
        break;
      }
    }
    m_params->rwLock->unlock();
    PlotPoint* pp;
    for (int i = 0; i < m_plotObject->points().size(); i++) {
      pp = m_plotObject->points().at(i);
      if (pp->customData().toInt() == ind) {
        ui.plot_plot->blockSignals(true);
        ui.plot_plot->clearAndSelectPoint(pp);
        ui.plot_plot->blockSignals(false);
        return;
      }
    }
    // If not found, clear selection
    ui.plot_plot->blockSignals(true);
    ui.plot_plot->clearSelection();
    ui.plot_plot->blockSignals(false);
  }

}

#include "tab_plot.moc"
