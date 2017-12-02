/**********************************************************************
  TabProgress - Table showing the progress of the search

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

#include <globalsearch/tracker.h>
#include <globalsearch/ui/abstracttab.h>

#include "ui_tab_progress.h"

class QTimer;
class QMutex;

namespace GlobalSearch {
class Structure;
}

namespace GAPC {
class GAPCDialog;
class OptGAPC;
class ProtectedCluster;

class TabProgress : public GlobalSearch::AbstractTab
{
  Q_OBJECT

public:
  explicit TabProgress(GAPCDialog* parent, OptGAPC* p);
  virtual ~TabProgress();

  enum ProgressColumns
  {
    Gen = 0,
    Mol,
    JobID,
    Status,
    TimeElapsed,
    Enthalpy,
    Ancestry
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
  void selectMoleculeFromProgress(int, int, int, int);
  void highlightPC(GlobalSearch::Structure* s);
  void startTimer();
  void stopTimer();
  void progressContextMenu(QPoint);
  void restartJobProgress();
  void killPCProgress();
  void unkillPCProgress();
  void resetFailureCountProgress();
  void randomizeStructureProgress();
  void enableRowTracking() { rowTracking = true; };
  void disableRowTracking() { rowTracking = false; };

signals:
  void deleteJob(int);
  void updateStatus(int opt, int run, int queue, int fail);
  void infoUpdate();

private:
  Ui::Tab_Progress ui;
  QTimer* m_timer;
  QMutex* m_mutex;
  QMutex *m_update_mutex, *m_update_all_mutex;
  ProtectedCluster* m_context_pc;
  bool rowTracking;

  GlobalSearch::Tracker m_infoUpdateTracker;

  void restartJobProgress_(int incar);
  void killPCProgress_();
  void unkillPCProgress_();
  void resetFailureCountProgress_();
  void randomizeStructureProgress_();
};
}

#endif
