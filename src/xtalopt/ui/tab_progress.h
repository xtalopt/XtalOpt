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

#ifndef TAB_PROGRESS_H
#define TAB_PROGRESS_H

#include "ui_tab_progress.h"

#include <globalsearch/tracker.h>
#include <globalsearch/ui/abstracttab.h>

#include <QBrush>

class QDialog;
class QTimer;
class QMutex;

namespace Ui {
class XrdOptionsDialog;
}

namespace GlobalSearch {
class AbstractDialog;
class Structure;
}

namespace XtalOpt {
class XrdPlot;
class XtalOpt;
class Xtal;

struct XO_Prog_TableEntry
{
  int gen;
  int id;
  int jobID;
  double enthalpy;
  double volume;
  int FU;
  QString elapsed;
  QString parents;
  QString spg;
  QString status;
  QBrush brush;
  QBrush pen;
};

class TabProgress : public GlobalSearch::AbstractTab
{
  Q_OBJECT

public:
  explicit TabProgress(GlobalSearch::AbstractDialog* parent, XtalOpt* p);
  virtual ~TabProgress() override;

  enum ProgressColumns
  {
    Gen = 0,
    Mol,
    JobID,
    Status,
    TimeElapsed,
    Enthalpy,
    FU,
    Volume,
    SpaceGroup,
    Ancestry
  };

public slots:
  void readSettings(const QString& filename = "") override;
  void writeSettings(const QString& filename = "") override;
  void disconnectGUI() override;
  void addNewEntry();
  void newInfoUpdate(GlobalSearch::Structure*);
  void updateInfo();
  void updateAllInfo();
  void updateProgressTable();
  void setTableEntry(int row, const XO_Prog_TableEntry& e);
  void selectMoleculeFromProgress(int, int, int, int);
  void highlightXtal(GlobalSearch::Structure* s);
  void startTimer();
  void stopTimer();
  void progressContextMenu(QPoint);
  void restartJobProgress();
  void killXtalProgress();
  void unkillXtalProgress();
  void resetFailureCountProgress();
  void randomizeStructureProgress();
  void replaceWithOffspringProgress();
  void injectStructureProgress();
  void clipPOSCARProgress();
  void plotXrdProgress();
  void enableRowTracking() { rowTracking = true; };
  void disableRowTracking() { rowTracking = false; };
  void updateRank();
  void clearFiles();
  void printFile();
  // The signal "readOnlySessionStarted()" calls this function.
  // It enables column sorting when a read-only session is started.
  void setColumnSortingEnabled() { ui.table_list->setSortingEnabled(true); };

signals:
  void deleteJob(int);
  void updateStatus(int opt, int iad, int run, int queue, int fail);
  void infoUpdate();
  void updateTableEntry(int row, const XO_Prog_TableEntry& e);

private:
  Ui::Tab_Progress ui;
  Ui::XrdOptionsDialog* m_ui_xrdOptionsDialog;
  QDialog* m_xrdOptionsDialog;
  QDialog* m_xrdPlotDialog;
  XrdPlot* m_xrdPlot;
  QTimer* m_timer;
  QMutex* m_mutex;
  QMutex *m_update_mutex, *m_update_all_mutex;
  QMutex* m_context_mutex;
  Xtal* m_context_xtal;
  bool rowTracking;

  GlobalSearch::Tracker m_infoUpdateTracker;

  void updateInfo_();
  void restartJobProgress_(int incar);
  void killXtalProgress_();
  void unkillXtalProgress_();
  void resetFailureCountProgress_();
  void randomizeStructureProgress_();
  void replaceWithOffspringProgress_();
  void injectStructureProgress_(const QString& filename);
  void clipPOSCARProgress_();
};
}

#endif
