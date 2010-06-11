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

#include <randomdock/ui/tab_sys.h>

#include <randomdock/ui/dialog.h>
#include <randomdock/randomdock.h>
#include <globalsearch/macros.h>

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
    connect(m_dialog, SIGNAL(tabsReadSettings(const QString &)),
            this, SLOT(readSettings(const QString &)));
    connect(m_dialog, SIGNAL(tabsWriteSettings(const QString &)),
            this, SLOT(writeSettings(const QString &)));
    connect(m_dialog, SIGNAL(tabsUpdateGUI()),
            this, SLOT(updateGUI()));
    connect(m_dialog, SIGNAL(tabsDisconnectGUI()),
            this, SLOT(disconnectGUI()));
    connect(m_dialog, SIGNAL(tabsLockGUI()),
            this, SLOT(lockGUI()));

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
    writeSettings();
  }

  void TabSys::writeSettings(const QString &filename)
  {
    SETTINGS(filename);
    settings->beginGroup("randomdock/sys");
    const int VERSION = 1;
    settings->setValue("version",     VERSION);

    settings->setValue("file/path",		m_opt->filePath);
    settings->setValue("queue/launch",		m_opt->qsub);
    settings->setValue("queue/check",		m_opt->qstat);
    settings->setValue("queue/qdel",		m_opt->qdel);
    settings->setValue("remote/host",		m_opt->host);
    settings->setValue("remote/username",	m_opt->username);
    settings->setValue("remote/rempath",	m_opt->rempath);

    settings->endGroup();
    DESTROY_SETTINGS(filename);
  }

  void TabSys::readSettings(const QString &filename)
  {
    SETTINGS(filename);
    settings->beginGroup("randomdock/sys");
    int loadedVersion = settings->value("version", 0).toInt();

    ui.edit_path->setText(	settings->value("file/path",		"/tmp").toString());
    ui.edit_base->setText(	settings->value("file/base",		"/opt-").toString());
    ui.edit_launch->setText(	settings->value("queue/launch",		"qsub").toString());
    ui.edit_check->setText(	settings->value("queue/check",		"qstat").toString());
    ui.edit_qdel->setText(	settings->value("queue/qdel",		"qdel").toString());
    ui.edit_host->setText(	settings->value("remote/host",		"").toString());
    ui.edit_username->setText(	settings->value("remote/username",	"").toString());
    ui.edit_rempath->setText(	settings->value("remote/rempath",	"").toString());

    settings->endGroup();

    // Update config data
    switch (loadedVersion) {
    case 0:
    case 1:
    default:
      break;
    }

  }

  void TabSys::updateGUI()
  {
  }

  void TabSys::disconnectGUI()
  {
  }

  void TabSys::lockGUI()
  {
    ui.edit_path->setDisabled(true);
    ui.edit_base->setDisabled(true);
    ui.edit_launch->setDisabled(true);
    ui.edit_check->setDisabled(true);
    ui.edit_qdel->setDisabled(true);
    ui.edit_host->setDisabled(true);
    ui.edit_username->setDisabled(true);
    ui.edit_rempath->setDisabled(true);
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
