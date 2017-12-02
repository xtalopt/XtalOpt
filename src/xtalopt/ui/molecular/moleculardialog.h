/**********************************************************************
  MolecularDialog - the dialog for molecular XtalOpt

  Copyright (C) 2017 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef MOLECULAR_XTALOPT_DIALOG_H
#define MOLECULAR_XTALOPT_DIALOG_H

#include <globalsearch/ui/abstractdialog.h>

#include <QObject>

namespace Ui {
class MolecularXtalOptDialog;
}

namespace XtalOpt {
class Xtal;
class XtalOpt;
class TabMolecularInit;
class TabEdit;
class TabMolecularOpt;
class TabSys;
class TabProgress;
class TabPlot;
class TabLog;
class XtalOptTest;

class MolecularXtalOptDialog : public GlobalSearch::AbstractDialog
{
  Q_OBJECT

public:
  // Setting interactive to false will disable the tutorial popup
  explicit MolecularXtalOptDialog(QWidget* parent = 0,
                                  Qt::WindowFlags f = Qt::Window,
                                  bool interactive = true,
                                  XtalOpt* xtalopt = nullptr);
  virtual ~MolecularXtalOptDialog() override;

  void setMolecule(GlobalSearch::Molecule* molecule);

  // Set's the plot widget's parent to nullptr and shows the plot widget
  void beginPlotOnlyMode();

public slots:
  void saveSession() override;
  void showTutorialDialog() const;

private slots:
  void startSearch() override;

signals:

private:
  Ui::MolecularXtalOptDialog* ui;

  TabMolecularInit* m_tab_init;
  TabEdit* m_tab_edit;
  TabMolecularOpt* m_tab_opt;
  TabSys* m_tab_sys;
  TabProgress* m_tab_progress;
  TabPlot* m_tab_plot;
  TabLog* m_tab_log;

  XtalOptTest* m_test;
};
}

#endif
