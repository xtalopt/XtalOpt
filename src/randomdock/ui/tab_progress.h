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

#ifndef TAB_PROGRESS_H
#define TAB_PROGRESS_H

#include "ui_tab_progress.h"

#include <globalsearch/tracker.h>
#include <globalsearch/ui/abstracttab.h>

#include <QBrush>

class QTimer;
class QMutex;

namespace GlobalSearch {
class Structure;
}

namespace RandomDock {
class RandomDockDialog;
class RandomDock;
class Scene;

struct RD_Prog_TableEntry
{
  int rank;
  int id;
  int jobID;
  double energy;
  QString elapsed;
  QString status;
  QBrush brush;
};

class TabProgress : public GlobalSearch::AbstractTab
{
  Q_OBJECT

public:
  explicit TabProgress(RandomDockDialog* parent, RandomDock* p);
  virtual ~TabProgress();

  enum ProgressColumns
  {
    C_Rank = 0,
    C_Index,
    C_Energy,
    C_Elapsed,
    C_JobID,
    C_Status
  };

public slots:
  void readSettings(const QString& filename = "");
  void writeSettings(const QString& filename = "");
  void disconnectGUI();
  void addNewEntry();
  void newInfoUpdate(GlobalSearch::Structure*);
  void updateInfo();
  void updateAllInfo();
  void updateProgressTable();
  void setTableEntry(int row, const RD_Prog_TableEntry& e);
  void selectMoleculeFromProgress(int, int, int, int);
  void highlightScene(GlobalSearch::Structure* structure);
  void startTimer();
  void stopTimer();
  void progressContextMenu(QPoint);
  void restartJobProgress();
  void killSceneProgress();
  void unkillSceneProgress();
  void resetFailureCountProgress();
  void randomizeStructureProgress();
  void enableRowTracking() { rowTracking = true; };
  void disableRowTracking() { rowTracking = false; };

signals:
  void deleteJob(int);
  void updateStatus(int opt, int run, int queue, int fail);
  void infoUpdate();
  void updateTableEntry(int row, const RD_Prog_TableEntry& e);

private:
  Ui::Tab_Progress ui;
  QTimer* m_timer;
  QMutex* m_mutex;
  QMutex* m_update_mutex;
  QMutex* m_update_all_mutex;
  Scene* m_context_scene;
  bool rowTracking;

  GlobalSearch::Tracker m_infoUpdateTracker;

  void updateInfo_();
  void restartJobProgress_(int incar);
  void killSceneProgress_();
  void unkillSceneProgress_();
  void resetFailureCountProgress_();
  void randomizeStructureProgress_();
};
}

#endif
