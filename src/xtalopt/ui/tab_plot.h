/**********************************************************************
  XtalOpt - Tools for advanced crystal optimization

  Copyright (C) 2009-2010 by David Lonie

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

namespace Avogadro {
  class PlotPoint;
}

using namespace Avogadro;

namespace XtalOpt {
  class XtalOptDialog;
  class XtalOpt;
  class Xtal;

  class TabPlot : public QObject
  {
    Q_OBJECT

  public:
    explicit TabPlot( XtalOptDialog *parent, XtalOpt *p );
    virtual ~TabPlot();

    enum PlotAxes {
      Structure_T = 0,
      Generation_T,
      Enthalpy_T,
      Energy_T,
      PV_T,
      A_T,
      B_T,
      C_T,
      Alpha_T,
      Beta_T,
      Gamma_A,
      Volume_T
    };

    enum PlotType {
      Trend_PT = 0,
      DistHist_PT
    };

    enum LabelTypes {
      Number_L = 0,
      Symbol_L,
      Enthalpy_L,
      Energy_L,
      PV_L,
      Volume_L,
      Generation_L,
      Structure_L
    };

    QWidget *getTabWidget() {return m_tab_widget;};

  public slots:
    // used to lock bits of the GUI that shouldn't be change when a
    // session starts. This will also pass the call on to all tabs.
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
    void populateXtalList();
    void selectMoleculeFromPlot(PlotPoint *pp);
    void selectMoleculeFromIndex(int index);
    void highlightXtal(Xtal* xtal);

  signals:
    void moleculeChanged(Xtal*);

  private:
    Ui::Tab_Plot ui;
    QWidget *m_tab_widget;
    XtalOptDialog *m_dialog;
    XtalOpt *m_opt;
    QReadWriteLock *m_plot_mutex;
    PlotObject *m_plotObject;
  };
}

#endif
