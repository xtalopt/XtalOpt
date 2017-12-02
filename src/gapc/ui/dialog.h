/**********************************************************************
  GAPC -- A genetic algorithm for protected clusters

  Copyright (C) 2010-2011 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef GAPCDIALOG_H
#define GAPCDIALOG_H

#include <QDialog>
#include <QMutex>
#include <QTimer>

#include <globalsearch/ui/abstractdialog.h>

#include <avogadro/glwidget.h>
#include <avogadro/molecule.h>

#include "ui_dialog.h"

namespace GAPC {
class ProtectedCluster;
class GAPC;
class TabInit;
class TabEdit;
class TabOpt;
class TabSys;
class TabProgress;
class TabPlot;
class TabLog;

class GAPCDialog : public GlobalSearch::AbstractDialog
{
  Q_OBJECT

public:
  explicit GAPCDialog(Avogadro::GLWidget* glWidget = 0, QWidget* parent = 0,
                      Qt::WindowFlags f = 0);
  virtual ~GAPCDialog();

  void setMolecule(Avogadro::Molecule* molecule);

public slots:
  void saveSession();

private slots:
  void startSearch();

signals:

private:
  Ui::GAPCDialog ui;

  TabInit* m_tab_init;
  TabEdit* m_tab_edit;
  TabOpt* m_tab_opt;
  TabSys* m_tab_sys;
  TabProgress* m_tab_progress;
  TabPlot* m_tab_plot;
  TabLog* m_tab_log;
};
}

#endif
