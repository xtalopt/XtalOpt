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

#ifndef RDDIALOG_H
#define RDDIALOG_H

#include <randomdock/randomdock.h>

#include <globalsearch/ui/abstractdialog.h>

#include "ui_dialog.h"

namespace Avogadro {
class PlotObject;
class Molecule;
class GLWidget;
}

namespace RandomDock {
class TabInit;
class TabConformers;
class TabEdit;
class TabParams;
class TabSys;
class TabProgress;
class TabPlot;
class TabLog;
class Scene;
class RandomDock;

class RandomDockDialog : public GlobalSearch::AbstractDialog
{
  Q_OBJECT

public:
  explicit RandomDockDialog(Avogadro::GLWidget* glWidget = 0,
                            QWidget* parent = 0, Qt::WindowFlags f = 0);
  virtual ~RandomDockDialog();

public slots:
  void saveSession();

private slots:
  void startSearch();

signals:

private:
  Ui::RandomDockDialog ui;

  TabInit* m_tab_init;
  TabConformers* m_tab_conformers;
  TabEdit* m_tab_edit;
  TabParams* m_tab_params;
  TabSys* m_tab_sys;
  TabProgress* m_tab_progress;
  TabPlot* m_tab_plot;
  TabLog* m_tab_log;
};
}

#endif
