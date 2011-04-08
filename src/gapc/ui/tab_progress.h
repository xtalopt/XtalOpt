/**********************************************************************
  TabProgress - Table showing the progress of the search

  Copyright (C) 2009-2011 by David Lonie

  This library is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as
  published by the Free Software Foundation; either version 2.1 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public icense for more details.
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
    explicit TabProgress( GAPCDialog *parent, OptGAPC *p );
    virtual ~TabProgress();

    enum ProgressColumns {
      Gen = 0,
      Mol,
      JobID,
      Status,
      TimeElapsed,
      Enthalpy,
      Ancestry
    };

  public slots:
    void readSettings(const QString &filename = "");
    void writeSettings(const QString &filename = "");
    void disconnectGUI();
    void addNewEntry();
    void newInfoUpdate(GlobalSearch::Structure *);
    void updateInfo();
    void updateAllInfo();
    void updateProgressTable();
    void selectMoleculeFromProgress(int,int,int,int);
    void highlightPC(GlobalSearch::Structure *s);
    void startTimer();
    void stopTimer();
    void progressContextMenu(QPoint);
    void restartJobProgress();
    void killPCProgress();
    void unkillPCProgress();
    void resetFailureCountProgress();
    void randomizeStructureProgress();
    void enableRowTracking() {rowTracking = true;};
    void disableRowTracking() {rowTracking = false;};

  signals:
    void deleteJob(int);
    void updateStatus(int opt, int run, int queue, int fail);
    void infoUpdate();

  private:
    Ui::Tab_Progress ui;
    QTimer *m_timer;
    QMutex *m_mutex;
    QMutex *m_update_mutex, *m_update_all_mutex;
    ProtectedCluster *m_context_pc;
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
