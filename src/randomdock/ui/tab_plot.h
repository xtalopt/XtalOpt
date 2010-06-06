/**********************************************************************
  RandomDock -- A tool for analysing a matrix-substrate docking problem

  Copyright (C) 2009-2010 by David Lonie

  This file is part of the Avogadro molecular editor project.
  For more information, see <http://avogadro.openmolecules.net/>

  This library is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as
  published by the Free Software Foundation; either version 2.1 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public icense for more details.
 ***********************************************************************/

#ifndef TAB_PLOT_H
#define TAB_PLOT_H

#include "ui_tab_plot.h"

class QReadWriteLock;

namespace GlobalSearch {
  class Structure;
}

namespace Avogadro {
  class PlotPoint;
  class PlotObject;
}

using namespace GlobalSearch;
using namespace Avogadro;

namespace RandomDock {
  class RandomDockDialog;
  class RandomDock;
  class Scene;

  class TabPlot : public QObject
  {
    Q_OBJECT

  public:
    explicit TabPlot( RandomDockDialog *parent, RandomDock *p );
    virtual ~TabPlot();

    enum PlotAxes {
      Structure_T = 0,
      Energy_T,
    };

    enum PlotType {
      Trend_PT = 0,
      DistHist_PT
    };

    enum LabelTypes {
      Index_L = 0,
      Energy_L,
    };

    QWidget *getTabWidget() {return m_tab_widget;};

  public slots:
    void lockGUI();
    void readSettings(const QString &filename = "");
    void writeSettings(const QString &filename = "");
    void updateGUI();
    void disconnectGUI();
    void lockClearAndSelectPoint(PlotPoint *pp);
    void refreshPlot();
    void updatePlot();
    void plotTrends();
    void plotDistHist();
    void populateStructureList();
    void selectStructureFromPlot(PlotPoint *pp);
    void selectStructureFromIndex(int index);
    void highlightStructure(Structure *stucture);

  signals:
    void moleculeChanged(Structure*);

  private:
    Ui::Tab_Plot ui;
    QWidget *m_tab_widget;
    RandomDockDialog *m_dialog;
    RandomDock *m_opt;
    QReadWriteLock *m_plot_mutex;
    PlotObject *m_plotObject;
  };
}

#endif
