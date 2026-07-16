/**********************************************************************
  TabProgress - The progress tab: the live table of structures and their current states.

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

#include <search/tracker.h>
#include <search/gui/abstracttab.h>

#include <QBrush>
#include <QPointer>
#include <QThreadPool>

#include <atomic>
#include <memory>

class QTimer;
class QMutex;

namespace Search {
class AbstractDialog;
class Structure;
}

namespace XtalOpt {
class XtalOpt;
class Xtal;

struct XO_Prog_TableEntry
{
  QString tag;
  QString formula;
  int jobID;
  double enthalpy;
  double abovehull;
  int front;
  double volume;
  QString elapsed;
  QString parents;
  QString spg;
  QString status;
  QBrush brush;
  QBrush pen;
};

/**
 * The progress tab: the live table of structures and their current
 * states, with a right-click menu for per-structure actions.
 */
class TabProgress : public Search::AbstractTab
{
  Q_OBJECT

public:
  explicit TabProgress(Search::AbstractDialog* parent, XtalOpt* p);
  virtual ~TabProgress() override;

  enum ProgressColumns
  {
    Tag = 0,
    Formula,
    JobID,
    Status,
    TimeElapsed,
    Enthalpy,
    AboveHull,
    Front,
    Volume,
    SpaceGroup,
    // Keep this one the last; it's used as the
    //   max number of columns in .cpp file!
    Ancestry
  };

public slots:
  void lockGUI() override;
  void disconnectGUI() override;
  void addNewEntry();
  void newInfoUpdate(Search::Structure*);
  void updateInfo();
  void updateAllInfo();
  void updateProgressTable();
  void refreshHullFrontEntries();
  void setTableEntry(int row, const XO_Prog_TableEntry& e);
  void selectGeometryFromProgress(int, int, int, int);
  void highlightXtal(Search::Structure* s);
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
  void viewStructureProgress();
  void plotXrdProgress();
  void enableRowTracking() { rowTracking = true; };
  void disableRowTracking() { rowTracking = false; };
  void clearFiles();

signals:
  void infoUpdate();
  void updateTableEntry(int row, const XO_Prog_TableEntry& e);

private:
  Ui::Tab_Progress ui;
  std::unique_ptr<QTimer> m_timer;
  std::unique_ptr<QMutex> m_mutex;
  std::unique_ptr<QMutex> m_update_mutex;
  std::unique_ptr<QMutex> m_update_all_mutex;
  std::unique_ptr<QMutex> m_context_mutex;
  // Selected structure.
  QPointer<Xtal> m_context_xtal;
  bool rowTracking;

  // Current number of rows.
  std::atomic<int> m_tableRows;

  // Pool for background work.
  QThreadPool m_workerPool;

  Search::Tracker m_infoUpdateTracker;

  void updateInfo_();
  void restartJobProgress_(int incar);
  void killXtalProgress_();
  void unkillXtalProgress_();
  void resetFailureCountProgress_();
  void randomizeStructureProgress_();
  void replaceWithOffspringProgress_();
  void injectStructureProgress_(const QString& filename);
  void clipPOSCARProgress_();
  void clearFiles_();
};
}

#endif
