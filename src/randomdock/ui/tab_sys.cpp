/**********************************************************************
  RandomDock -- A tool for analysing a matrix-substrate docking problem

  Copyright (C) 2009 by David Lonie

  This library is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as
  published by the Free Software Foundation; either version 2.1 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include "tab_sys.h"

#include "dialog.h"
#include "../randomdock.h"

#include <QSettings>

using namespace std;
using namespace Avogadro;

namespace RandomDock {

  TabSys::TabSys( RandomDockDialog *dialog, RandomDock *opt ) :
    QObject(dialog),
    m_dialog(dialog),
    m_opt(opt)
  {
    m_tab_widget = new QWidget;
    ui.setupUi(m_tab_widget);

    // dialog connections
    connect(m_dialog, SIGNAL(tabsReadSettings()),
            this, SLOT(readSettings()));
    connect(m_dialog, SIGNAL(tabsWriteSettings()),
            this, SLOT(writeSettings()));

    // System Settings connections
    connect(ui.edit_path, SIGNAL(textChanged(QString)),
            this, SLOT(updateSystemInfo()));
    connect(ui.edit_base, SIGNAL(textChanged(QString)),
            this, SLOT(updateSystemInfo()));
    connect(ui.edit_launch, SIGNAL(textChanged(QString)),
            this, SLOT(updateSystemInfo()));
    connect(ui.edit_check, SIGNAL(textChanged(QString)),
            this, SLOT(updateSystemInfo()));
    connect(ui.edit_qdel, SIGNAL(textChanged(QString)),
            this, SLOT(updateSystemInfo()));
    connect(ui.edit_host, SIGNAL(textChanged(QString)),
            this, SLOT(updateSystemInfo()));
    connect(ui.edit_username, SIGNAL(textChanged(QString)),
            this, SLOT(updateSystemInfo()));
    connect(ui.edit_rempath, SIGNAL(textChanged(QString)),
            this, SLOT(updateSystemInfo()));
  }

  TabSys::~TabSys()
  {
    qDebug() << "TabSys::~TabSys() called";
    writeSettings();
  }

  void TabSys::writeSettings() {
    qDebug() << "TabSys::writeSettings() called";
    QSettings settings; // Already set up in avogadro/src/main.cpp

    settings.setValue("randomdock/dialog/sys/file/path",	m_opt->filePath);
    settings.setValue("randomdock/dialog/sys/queue/launch",	m_opt->qsub);
    settings.setValue("randomdock/dialog/sys/queue/check",	m_opt->qstat);
    settings.setValue("randomdock/dialog/sys/queue/qdel",	m_opt->qdel);
    settings.setValue("randomdock/dialog/sys/remote/host",	m_opt->host);
    settings.setValue("randomdock/dialog/sys/remote/username",	m_opt->username);
    settings.setValue("randomdock/dialog/sys/remote/rempath",	m_opt->rempath);
  }

  void TabSys::readSettings() {
    qDebug() << "TabSys::readSettings() called";
    QSettings settings; // Already set up in avogadro/src/main.cpp

    ui.edit_path->setText(	settings.value("randomdock/dialog/sys/file/path",		"/tmp").toString());
    ui.edit_base->setText(	settings.value("randomdock/dialog/sys/file/base",		"/opt-").toString());
    ui.edit_launch->setText(	settings.value("randomdock/dialog/sys/queue/launch",		"qsub").toString());
    ui.edit_check->setText(	settings.value("randomdock/dialog/sys/queue/check",		"qstat").toString());
    ui.edit_qdel->setText(	settings.value("randomdock/dialog/sys/queue/qdel",		"qdel").toString());
    ui.edit_host->setText(	settings.value("randomdock/dialog/sys/remote/host",		"").toString());
    ui.edit_username->setText(	settings.value("randomdock/dialog/sys/remote/username",		"").toString());
    ui.edit_rempath->setText(	settings.value("randomdock/dialog/sys/remote/rempath",		"").toString());
    ui.cb_remote->setChecked(	settings.value("randomdock/dialog/using/remote",		false).toBool());
  }

  void TabSys::updateSystemInfo() {
    m_opt->filePath		= ui.edit_path->text();
    m_opt->qsub			= ui.edit_launch->text();
    m_opt->qstat		= ui.edit_check->text();
    m_opt->qdel			= ui.edit_qdel->text();
    m_opt->host			= ui.edit_host->text();
    m_opt->username		= ui.edit_username->text();
    m_opt->rempath		= ui.edit_rempath->text();
  }

}

//#include "tab_sys.moc"
