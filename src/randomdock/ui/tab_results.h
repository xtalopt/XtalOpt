/**********************************************************************
  RandomDock -- A tool for analysing a matrix-substrate docking problem

  Copyright (C) 2009 by David Lonie

  This file is part of the Avogadro molecular editor project.
  For more information, see <http://avogadro.openmolecules.net/>

  This library is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as
  published by the Free Software Foundation; either version 2.1 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public icense for more details.
 ***********************************************************************/

#ifndef TAB_RESULTS_H
#define TAB_RESULTS_H

#include "ui_tab_results.h"

#include <avogadro/molecule.h>

class QTimer;
class QMutex;

namespace Avogadro {
  class RandomDockDialog;
  class RandomDockParams;
  class Scene;

  class TabResults : public QObject
  {
    Q_OBJECT

  public:
    explicit TabResults( RandomDockParams *p );
    virtual ~TabResults();

    enum Columns {Rank, Index, Energy, Status};

    QWidget *getTabWidget() {return m_tab_widget;};

  public slots:
    void readSettings();
    void writeSettings();
    void populateResultsTable();
    void selectSceneFromResults(int,int,int,int);
    void highlightScene(Molecule *mol);
    void startTimer();
    void stopTimer();
    void updateResultsTable();
    void refreshResultsTable();
    void dumpResults();

  signals:
    void newLog(const QString &);
    void moleculeChanged(Molecule*);
    void refresh();
    void sceneFinished(int);
    void sceneErrored(int);
    void sceneStarted(int);
    void sceneStepOptimized(int);
    void deleteJob(int);
    void killScene(int);
    void cutoffReached();
    void updateRunning(int);
    void updateOptimized(int);

  private:
    Ui::Tab_Results ui;
    QWidget *m_tab_widget;
    RandomDockParams *m_params;
    QTimer *m_timer;
    QMutex *m_update_mutex;
    QMutex *m_mutex;
  };
}

#endif
