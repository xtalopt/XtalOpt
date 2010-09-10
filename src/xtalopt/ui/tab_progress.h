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
#include <globalsearch/ui/abstracttab.h>

#include <QtGui/QBrush>

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

  class TabProgress : public AbstractTab
  {
    Q_OBJECT

  public:
    explicit TabProgress( XtalOptDialog *parent, XtalOpt *p );
    virtual ~TabProgress();

    enum ProgressColumns {
      Gen = 0,
      Mol,
      JobID,
      Status,
      TimeElapsed,
      Enthalpy,
      Volume,
      SpaceGroup,
      Ancestry
    };

    struct TableEntry {
      int gen;
      int id;
      int jobID;
      double enthalpy;
      double volume;
      QString elapsed;
      QString parents;
      QString spg;
      QString status;
      QBrush brush;
    };

  public slots:
    void readSettings(const QString &filename = "");
    void writeSettings(const QString &filename = "");
    void disconnectGUI();
    void addNewEntry();
    void newInfoUpdate(Structure *);
    void updateInfo();
    void updateAllInfo();
    void updateProgressTable();
    void setTableEntry(int row, const TableEntry& e);
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
    void refresh();
    void deleteJob(int);
    void updateStatus(int opt, int run, int queue, int fail);
    void infoUpdate();
    void updateTableEntry(int row, const TableEntry& e);

  private:
    Ui::Tab_Progress ui;
    QTimer *m_timer;
    QMutex *m_mutex;
    QMutex *m_update_mutex, *m_update_all_mutex;
    QMutex *m_context_mutex;
    Xtal *m_context_xtal;
    bool rowTracking;

    Tracker m_infoUpdateTracker;

    void updateInfo_();
    void restartJobProgress_(int incar);
    void killXtalProgress_();
    void unkillXtalProgress_();
    void resetFailureCountProgress_();
    void randomizeStructureProgress_();
  };
}

#endif
