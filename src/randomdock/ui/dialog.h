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

#ifndef RANDOMDOCKDIALOG_H
#define RANDOMDOCKDIALOG_H

#include "templates.h"

#include <QDialog>
#include <QMutex>

#include <avogadro/primitive.h>
#include <avogadro/molecule.h>
#include <avogadro/atom.h>

#include "ui_randomdockdialog.h"

namespace Avogadro {
  class PlotObject;
  class TabInit;
  class TabConformers;
  class TabEdit;
  class TabParams;
  class TabSys;
  class TabResults;
  class TabPlot;
  class TabLog;

  class RandomDockDialog : public QDialog
  {
    Q_OBJECT

  public:
    explicit RandomDockDialog( QWidget *parent = 0, Qt::WindowFlags f = 0 );
    virtual ~RandomDockDialog();

    void writeSettings();
    void readSettings();
    Molecule* getMolecule() {return m_molecule;};

    // TODO: Move this back to private after setting up signals/slot in progress update
    TabResults *m_tab_results;

  public slots:
    void saveSession();
    void log(const QString &str) {emit newLog(str);};
    void updateScene(int ind);
    void errorScene(int ind);
    void deleteJob(int ind);
    void killScene(int ind);
    void updateRunning(int i);
    void updateOptimized(int i);

  private slots:
    void startSearch();
    void resumeSession();

  signals:
    void moleculeChanged(Molecule*);
    void sceneChanged(Scene*);
    void tabsWriteSettings();
    void tabsReadSettings();
    void newLog(const QString &str);

  private:
    Ui::RandomDockDialog ui;

    TabInit *m_tab_init;
    TabConformers *m_tab_conformers;
    TabEdit *m_tab_edit;
    TabParams *m_tab_params;
    TabSys *m_tab_sys;
    TabPlot *m_tab_plot;
    TabLog *m_tab_log;

    RandomDockParams *m_params;
    Molecule *m_molecule;

    void errorScene_(int ind);
    void killScene_(int ind);
  };
}

#endif
