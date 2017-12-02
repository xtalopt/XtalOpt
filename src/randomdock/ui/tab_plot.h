/**********************************************************************
  RandomDock -- A tool for analysing a matrix-substrate docking problem

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
class PlotObject;
}

// Workaround for Qt's ignorance of namespaces in signals/slots
using Avogadro::PlotPoint;

namespace RandomDock {
class RandomDockDialog;
class RandomDock;
class Scene;

class TabPlot : public GlobalSearch::AbstractTab
{
  Q_OBJECT

public:
  explicit TabPlot(RandomDockDialog* parent, RandomDock* p);
  virtual ~TabPlot();

  enum PlotAxes
  {
    Structure_T = 0,
    Energy_T,
  };

  enum PlotType
  {
    Trend_PT = 0,
    DistHist_PT
  };

  enum LabelTypes
  {
    Index_L = 0,
    Energy_L,
  };

public slots:
  void readSettings(const QString& filename = "");
  void writeSettings(const QString& filename = "");
  void updateGUI();
  void disconnectGUI();
  void lockClearAndSelectPoint(PlotPoint* pp);
  void selectStructureFromPlot(PlotPoint* pp);
  void refreshPlot();
  void updatePlot();
  void plotTrends();
  void plotDistHist();
  void populateStructureList();
  void selectStructureFromIndex(int index);
  void highlightStructure(GlobalSearch::Structure* stucture);

signals:

private:
  Ui::Tab_Plot ui;
  QReadWriteLock* m_plot_mutex;
  Avogadro::PlotObject* m_plotObject;
};
}

#endif
