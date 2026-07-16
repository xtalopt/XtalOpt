/**********************************************************************
  XtalOptDialog - The main GUI window: creates the tabs and connects them to the search engine.

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

#include <search/gui/abstractdialog.h>

#include <QObject>
#include <QCloseEvent>
#include <QKeyEvent>

namespace Ui {
/**
 * The main window of the GUI. Creates the tabs, connects them to the
 * XtalOpt search object, and starts or resumes sessions. All real
 * work happens in the core; this class only displays and forwards.
 */
class XtalOptDialog;
}

namespace Search {
class Structure;
}

namespace XtalOpt {
class Xtal;
class XtalOpt;
class TabStruc;
class TabOpt;
class TabSearch;
class TabMo;
class TabProgress;
class TabPlot;
class TabLog;
class TabAbout;

class XtalOptDialog : public Search::AbstractDialog
{
  Q_OBJECT

public:
  // The interactive flag is no longer used; the signature is kept so
  //   existing code that creates the dialog still works.
  explicit XtalOptDialog(QWidget* parent = 0, Qt::WindowFlags f = Qt::Window,
                         bool interactive = true, XtalOpt* xtalopt = nullptr);
  virtual ~XtalOptDialog() override;

  // Set's the plot widget's parent to nullptr and shows the plot widget
  void beginPlotOnlyMode();

public slots:
  void lockGUI() override;
  void saveSession() override;
  bool importSettings();
  bool exportSettings();
  void errorPromptWindow(const QString& instr);
  void showStructureViewer(Search::Structure* structure);
  void showXrdViewer(Search::Structure* structure);

private slots:
  void startSearch() override;
  void closeEvent(QCloseEvent *e) override;
  void keyPressEvent(QKeyEvent *e) override;

signals:

private:
  QString sessionStateFilePath() const;
  bool prepareResumeSession(const QString& filename) override;
  void resumeSession_(const QString& filename) override;

  Ui::XtalOptDialog* ui;
  QPushButton* ui_push_import;
  QPushButton* ui_push_export;

  TabStruc* m_tab_struc;
  TabOpt* m_tab_opt;
  TabSearch* m_tab_search;
  TabMo* m_tab_mo;
  TabProgress* m_tab_progress;
  TabPlot* m_tab_plot;
  TabLog* m_tab_log;
  TabAbout* m_tab_about;
};
}

#endif
