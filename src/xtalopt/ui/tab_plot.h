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
  GNU General Public icense for more details.
 ***********************************************************************/

#ifndef TAB_PLOT_H
#define TAB_PLOT_H

#include <globalsearch/ui/abstracttab.h>

#include "ui_tab_plot.h"

class QReadWriteLock;

namespace GlobalSearch {
  class Structure;
}

namespace Avogadro {
  class PlotPoint;
}

namespace XtalOpt {
  class XtalOptDialog;
  class XtalOpt;
  class Xtal;

  class TabPlot : public GlobalSearch::AbstractTab
  {
    Q_OBJECT;
    // Workaround for Qt's ignorance of namespaces in signals/slots
    typedef Avogadro::PlotPoint PlotPoint;

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

  public slots:
    void readSettings(const QString &filename = "");
    void writeSettings(const QString &filename = "");
    void updateGUI();
    void disconnectGUI();
    // PlotPoint is typedef'd to Avogadro::PlotPoint above
    void lockClearAndSelectPoint(PlotPoint *pp);
    void selectMoleculeFromPlot(double x, double y);
    void selectMoleculeFromPlot(PlotPoint *pp);
    void refreshPlot();
    void updatePlot();
    void plotTrends();
    void plotDistHist();
    void populateXtalList();
    void selectMoleculeFromIndex(int index);
    void highlightXtal(GlobalSearch::Structure *s);

  private:
    Ui::Tab_Plot ui;
    QReadWriteLock *m_plot_mutex;
    Avogadro::PlotObject *m_plotObject;
  };
}

#endif
