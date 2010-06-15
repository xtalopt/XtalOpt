/**********************************************************************
  XtalOpt - Tools for advanced crystal optimization

  Copyright (C) 2009-2010 by David Lonie

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

class QTimer;
class QMutex;

namespace GlobalSearch {
  class Structure;
}

using namespace GlobalSearch;

namespace XtalOpt {
  class XtalOptDialog;
  class XtalOpt;
  class Xtal;

  class TabProgress : public QObject
  {
    Q_OBJECT

  public:
    explicit TabProgress( XtalOptDialog *parent, XtalOpt *p );
    virtual ~TabProgress();

    enum ProgressColumns	{Gen = 0, Mol, JobID, Status, TimeElapsed, Enthalpy, Volume, SpaceGroup, Ancestry};

    QWidget *getTabWidget() {return m_tab_widget;};

  public slots:
    // used to lock bits of the GUI that shouldn't be change when a
    // session starts. This will also pass the call on to all tabs.
    void lockGUI();
    void readSettings(const QString &filename = "");
    void writeSettings(const QString &filename = "");
    void updateGUI();
    void disconnectGUI();
    void addNewEntry();
    void newInfoUpdate(Structure *);
    void updateInfo();
    void updateAllInfo();
    void updateProgressTable();
    void selectMoleculeFromProgress(int,int,int,int);
    void highlightXtal(Structure *s);
    void startTimer();
    void stopTimer();
    void progressContextMenu(QPoint);
    void restartJobProgress();
    void killXtalProgress();
    void unkillXtalProgress();
    void resetFailureCountProgress();
    void randomizeStructureProgress();
    void enableRowTracking() {rowTracking = true;};
    void disableRowTracking() {rowTracking = false;};

  signals:
    void newLog(const QString &);
    void moleculeChanged(Structure*);
    void refresh();
    void deleteJob(int);
    void updateStatus(int opt, int run, int queue, int fail);
    void infoUpdate();

  private:
    Ui::Tab_Progress ui;
    QWidget *m_tab_widget;
    XtalOptDialog *m_dialog;
    XtalOpt *m_opt;
    QTimer *m_timer;
    QMutex *m_mutex;
    QMutex *m_update_mutex, *m_update_all_mutex;
    Xtal *m_context_xtal;
    bool rowTracking;

    Tracker m_infoUpdateTracker;

    void restartJobProgress_(int incar);
    void killXtalProgress_();
    void unkillXtalProgress_();
    void resetFailureCountProgress_();
    void randomizeStructureProgress_();
  };
}

#endif
