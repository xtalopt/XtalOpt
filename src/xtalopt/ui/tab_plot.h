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

#ifndef TAB_PLOT_H
#define TAB_PLOT_H

#include <atomic>

#include <globalsearch/ui/abstracttab.h>

#include "ui_tab_plot.h"

class QReadWriteLock;

namespace GlobalSearch {
class AbstractDialog;
class Structure;
}

namespace XtalOpt {
class XtalOpt;
class Xtal;

class TabPlot : public GlobalSearch::AbstractTab
{
  Q_OBJECT

public:
  explicit TabPlot(GlobalSearch::AbstractDialog* parent, XtalOpt* p);
  virtual ~TabPlot() override;

  enum PlotAxes
  {
    StructureINDX_T = 0,
    AboveHull_per_Atm_T,
    Enthalpy_per_Atm_T,
    Volume_per_Atm_T,
    ParetoFront_T,
    PV_T,
    Enthalpy_T,
    Energy_T,
    Volume_T,
    Generation_T,
    A_T,
    B_T,
    C_T,
    Alpha_T,
    Beta_T,
    Gamma_A,
    // The objective entries: these will appear in the menu only
    //   if it's a multi-objective run.
    // Note: since the number of objectives is not fixed, we add
    //   their entry at the end: right after the last well-defined one.
    //   (1) Any non-objective entry should be added before this line!
    //   (2) Don't ever add any entries after this line!
    Objectivei_T
    //
  };

  enum PlotType
  {
    Trend_PT = 0,
    DistHist_PT
  };

  enum LabelTypes
  {
    StructureTAG_L = 0,
    StructureINDX_L,
    Number_L,
    Symbol_L,
    ParetoFront_L,
    AboveHull_per_Atm_L,
    Enthalpy_per_Atm_L,
    Volume_per_Atm_L,
    PV_L,
    Enthalpy_L,
    Energy_L,
    Generation_L,
    // The objective entries: these will appear in the menu only
    //   if it's a multi-objective run.
    // Note: since the number of objectives is not fixed, we add
    //   their entry at the end: right after the last well-defined one.
    //   (1) Any non-objective entry should be added before this line!
    //   (2) Don't ever add any entries after this line!
    Objectivei_L
    //
  };

public slots:
  void selectXtal(QwtPlotMarker* pm);
  void readSettings(const QString& filename = "") override;
  void writeSettings(const QString& filename = "") override;
  void updateGUI() override;
  void disconnectGUI() override;
  void enablePlotUpdate() { m_enablePlotUpdate = true; };
  void disablePlotUpdate() { m_enablePlotUpdate = false; };
  void refreshPlot();
  void updatePlot();
  void plotTrends();
  void plotDistHist();
  void selectMoleculeFromIndex(int index);
  void highlightXtal(GlobalSearch::Structure* s);

private:
  QwtPlotMarker* addXtalToPlot(Xtal* xtal, double x, double y);
  void plotTrace(double x1, double y1, double x2, double y2);

  std::atomic_bool m_enablePlotUpdate;

  Ui::Tab_Plot ui;
  QReadWriteLock* m_plot_mutex;
  QMap<QwtPlotMarker*, Xtal*> m_marker_xtal_map;
};
}

#endif
