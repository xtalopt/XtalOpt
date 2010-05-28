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

#include "randomdock.h"
#include "randomdockdialog.h"

#include <QSettings>

using namespace std;

namespace Avogadro {

  TabSys::TabSys( RandomDockParams *p ) :
    QObject( p->dialog ), m_params(p)
  {
    qDebug() << "TabSys::TabSys( " << p <<  " ) called.";

    m_tab_widget = new QWidget;
    ui.setupUi(m_tab_widget);

    // dialog connections
    connect(p->dialog, SIGNAL(tabsReadSettings()),
            this, SLOT(readSettings()));
    connect(p->dialog, SIGNAL(tabsWriteSettings()),
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
    connect(ui.cb_remote, SIGNAL(toggled(bool)),
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

    settings.setValue("randomdock/dialog/sys/file/path",	m_params->filePath);
    settings.setValue("randomdock/dialog/sys/file/base",	m_params->fileBase);
    settings.setValue("randomdock/dialog/sys/queue/launch",	m_params->launchCommand);
    settings.setValue("randomdock/dialog/sys/queue/check",	m_params->queueCheck);
    settings.setValue("randomdock/dialog/sys/queue/qdel",	m_params->queueDelete);
    settings.setValue("randomdock/dialog/sys/remote/host",	m_params->host);
    settings.setValue("randomdock/dialog/sys/remote/username",	m_params->username);
    settings.setValue("randomdock/dialog/sys/remote/rempath",	m_params->rempath);
    settings.setValue("randomdock/dialog/using/remote",    	m_params->using_remote);
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
    qDebug() << "TabSys::updateSystemInfo() called";
    m_params->filePath		= ui.edit_path->text();
    m_params->fileBase		= ui.edit_base->text();
    m_params->launchCommand	= ui.edit_launch->text();
    m_params->queueCheck	= ui.edit_check->text();
    m_params->queueDelete	= ui.edit_qdel->text();
    m_params->using_remote	= ui.cb_remote->isChecked();
    m_params->host		= ui.edit_host->text();
    m_params->username		= ui.edit_username->text();
    m_params->rempath		= ui.edit_rempath->text();
  }

}

//#include "tab_sys.moc"
