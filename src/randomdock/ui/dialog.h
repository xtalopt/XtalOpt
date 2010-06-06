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

#ifndef RDDIALOG_H
#define RDDIALOG_H

#include <QDialog>
#include <QMutex>

#include "ui_dialog.h"

namespace Avogadro {
  class PlotObject;
  class Molecule;
  class Structure;
  class GLWidget;
}

using namespace Avogadro;

namespace RandomDock {
  class TabInit;
  class TabConformers;
  class TabEdit;
  class TabParams;
  class TabSys;
  class TabProgress;
  class TabPlot;
  class TabLog;
  class Scene;
  class RandomDock;

  class RandomDockDialog : public QDialog
  {
    Q_OBJECT

  public:
    explicit RandomDockDialog( GLWidget *glWidget = 0, QWidget *parent = 0, Qt::WindowFlags f = 0 );
    virtual ~RandomDockDialog();

    Molecule* setMolecule(Molecule *mol) {Q_UNUSED(mol);};
    Molecule* getMolecule() {return m_molecule;};
    GLWidget* getGLWidget() {return m_glWidget;};
    RandomDock* getRandomDock() {return m_opt;};

  public slots:
    // used for testing. You probably don't want to call this.
    void disconnectGUI();
    // used to lock bits of the GUI that shouldn't be change when a
    // session starts. This will also pass the call on to all tabs.
    void lockGUI();
    void writeSettings(const QString &filename = "");
    void readSettings(const QString &filename = "");
    void saveSession();
    void log(const QString &str) {emit newLog(str);};
    void updateStatus(int opt, int run, int fail);
    void updateGUI();
    void setGLWidget(GLWidget *w) {m_glWidget = w;};
    void startProgressUpdate(const QString & text, int min, int max);
    void stopProgressUpdate();
    void updateProgressMinimum(int min);
    void updateProgressMaximum(int max);
    void updateProgressValue(int val);
    void updateProgressLabel(const QString & text);
    void repaintProgressBar();
    void newDebug(const QString &);
    void newWarning(const QString &);
    void newError(const QString &);
    void errorBox(const QString &);

  private slots:
    void startSearch();
    void resumeSession();
    void updateStatus_(int,int,int);
    void startProgressUpdate_(const QString & text, int min, int max);
    void stopProgressUpdate_();
    void updateProgressMinimum_(int min);
    void updateProgressMaximum_(int max);
    void updateProgressValue_(int val);
    void updateProgressLabel_(const QString & text);
    void repaintProgressBar_();
    void errorBox_(const QString &);

  signals:
    void tabsDisconnectGUI();
    void tabsLockGUI();
    void moleculeChanged(Structure*);
    void tabsWriteSettings(const QString &filename);
    void tabsReadSettings(const QString &filename);
    void tabsUpdateGUI();
    void newLog(const QString &str);
    void xtalReadyToSubmit();
    void optTypeChanged();
    void updateAllInfo();
    void sig_updateStatus(int,int,int);
    void sig_startProgressUpdate(const QString & text, int min, int max);
    void sig_stopProgressUpdate();
    void sig_updateProgressMinimum(int min);
    void sig_updateProgressMaximum(int max);
    void sig_updateProgressValue(int val);
    void sig_updateProgressLabel(const QString & text);
    void sig_repaintProgressBar();
    void sig_errorBox(const QString &);

  private:
    Ui::RandomDockDialog ui;

    TabInit *m_tab_init;
    TabConformers *m_tab_conformers;
    TabEdit *m_tab_edit;
    TabParams *m_tab_params;
    TabSys *m_tab_sys;
    TabProgress *m_tab_progress;
    TabPlot *m_tab_plot;
    TabLog *m_tab_log;

    Molecule *m_molecule;

    RandomDock *m_opt;
    GLWidget *m_glWidget;
    QMutex *progMutex;
    QTimer *progTimer;

  };
}

#endif
