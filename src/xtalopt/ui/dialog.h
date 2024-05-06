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

#ifndef XTALOPTDIALOG_H
#define XTALOPTDIALOG_H

#include <globalsearch/ui/abstractdialog.h>

#include <QObject>
#include <QCloseEvent>
#include <QKeyEvent>

namespace Ui {
class XtalOptDialog;
}

namespace XtalOpt {
class Xtal;
class XtalOpt;
class TabInit;
class TabEdit;
class TabOpt;
class TabMo;
class TabSys;
class TabProgress;
class TabPlot;
class TabLog;
class XtalOptTest;

class XtalOptDialog : public GlobalSearch::AbstractDialog
{
  Q_OBJECT

public:
  // Setting interactive to false will disable the tutorial popup
  explicit XtalOptDialog(QWidget* parent = 0, Qt::WindowFlags f = Qt::Window,
                         bool interactive = true, XtalOpt* xtalopt = nullptr);
  virtual ~XtalOptDialog() override;

  void setMolecule(GlobalSearch::Molecule* molecule);

  // Set's the plot widget's parent to nullptr and shows the plot widget
  void beginPlotOnlyMode();

public slots:
  void saveSession() override;
  void showTutorialDialog() const;

private slots:
  void startSearch() override;
  void closeEvent(QCloseEvent *e) override;
  void keyPressEvent(QKeyEvent *e) override;

signals:

private:
  Ui::XtalOptDialog* ui;

  TabInit* m_tab_init;
  TabEdit* m_tab_edit;
  TabOpt* m_tab_opt;
  TabMo* m_tab_mo;
  TabSys* m_tab_sys;
  TabProgress* m_tab_progress;
  TabPlot* m_tab_plot;
  TabLog* m_tab_log;

  XtalOptTest* m_test;
};
}

#endif
