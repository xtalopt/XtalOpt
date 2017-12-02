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

namespace GAPC {
class GAPCDialog;
class OptGAPC;
class ProtectedCluster;

class TabPlot : public GlobalSearch::AbstractTab
{
  Q_OBJECT
  // Workaround for Qt's ignorance of namespaces in signals/slots
  typedef Avogadro::PlotPoint PlotPoint;

public:
  explicit TabPlot(GAPCDialog* parent, OptGAPC* p);
  virtual ~TabPlot();

  enum PlotAxes
  {
    Structure_T = 0,
    Generation_T,
    Enthalpy_T
  };

  enum PlotType
  {
    Trend_PT = 0,
    DistHist_PT
  };

  enum LabelTypes
  {
    Structure_L = 0,
    Generation_L,
    Enthalpy_L
  };

public slots:
  void readSettings(const QString& filename = "");
  void writeSettings(const QString& filename = "");
  void updateGUI();
  void disconnectGUI();
  // PlotPoint is typedef'd to Avogadro::PlotPoint above
  void lockClearAndSelectPoint(PlotPoint* pp);
  void selectMoleculeFromPlot(PlotPoint* pp);
  void refreshPlot();
  void updatePlot();
  void plotTrends();
  void plotDistHist();
  void populatePCList();
  void selectMoleculeFromIndex(int index);
  void highlightPC(GlobalSearch::Structure* s);

private:
  Ui::Tab_Plot ui;
  QReadWriteLock* m_plot_mutex;
  Avogadro::PlotObject* m_plotObject;
};
}

#endif
