/**********************************************************************
  ExampleSearch -- A tool for analysing a matrix-substrate docking problem

  Copyright (C) 2012 by David C. Lonie

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

#include "ui_tab_progress.h"

#include <globalsearch/tracker.h>
#include <globalsearch/ui/abstracttab.h>

#include <QtGui/QBrush>

class QTimer;
class QMutex;

namespace GlobalSearch {
  class Structure;
}

namespace ExampleSearch {
  class ExampleSearchDialog;
  class ExampleSearch;

  struct ProgressTableEntry {
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
    explicit TabProgress( ExampleSearchDialog *parent, ExampleSearch *p );
    virtual ~TabProgress();

    enum ProgressColumns {
      C_Index = 0,
      C_Energy,
      C_Elapsed,
      C_JobID,
      C_Status
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
    void setTableEntry(int row, const ProgressTableEntry& e);
    void selectMoleculeFromProgress(int,int,int,int);
    void highlightStructure(GlobalSearch::Structure *structure);
    void startTimer();
    void stopTimer();
    void progressContextMenu(QPoint);
    void restartJobProgress();
    void killStructureProgress();
    void unkillStructureProgress();
    void resetFailureCountProgress();
    void randomizeStructureProgress();
    void enableRowTracking() {rowTracking = true;}
    void disableRowTracking() {rowTracking = false;}

  signals:
    void deleteJob(int);
    void updateStatus(int opt, int run, int queue, int fail);
    void infoUpdate();
    void updateTableEntry(int row, const ProgressTableEntry& e);

  private:
    Ui::Tab_Progress ui;
    QTimer *m_timer;
    QMutex *m_mutex;
    QMutex *m_update_mutex;
    QMutex *m_update_all_mutex;
    GlobalSearch::Structure *m_context_structure;
    bool rowTracking;

    GlobalSearch::Tracker m_infoUpdateTracker;

    void updateInfo_();
    void restartJobProgress_(int incar);
    void killStructureProgress_();
    void unkillStructureProgress_();
    void resetFailureCountProgress_();
    void randomizeStructureProgress_();
  };
}

#endif
