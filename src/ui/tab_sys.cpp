/**********************************************************************
  XtalOpt - Tools for advanced crystal optimization

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

#include <QSettings>

using namespace std;

namespace Avogadro {

  TabSys::TabSys( XtalOptDialog *parent, XtalOpt *p ) :
    QObject( parent ), m_dialog(parent), m_opt(p)
  {
    //qDebug() << "TabSys::TabSys( " << parent <<  " ) called.";

    m_tab_widget = new QWidget;
    ui.setupUi(m_tab_widget);

    m_dialog = parent;

    // dialog connections
    connect(m_dialog, SIGNAL(tabsReadSettings()),
            this, SLOT(readSettings()));
    connect(m_dialog, SIGNAL(tabsWriteSettings()),
            this, SLOT(writeSettings()));
    connect(m_dialog, SIGNAL(tabsUpdateGUI()),
            this, SLOT(updateGUI()));
    connect(m_dialog, SIGNAL(tabsDisconnectGUI()),
            this, SLOT(disconnectGUI()));
    connect(m_dialog, SIGNAL(tabsLockGUI()),
            this, SLOT(lockGUI()));
    connect(this, SIGNAL(dataChanged()),
            m_dialog, SLOT(updateGUI()));
    connect(m_dialog, SIGNAL(optTypeChanged()),
            this, SLOT(updateGUI()));

    // System Settings connections
    connect(ui.edit_path, SIGNAL(editingFinished()),
            this, SLOT(updateSystemInfo()));
    connect(ui.edit_base, SIGNAL(editingFinished()),
            this, SLOT(updateSystemInfo()));
    connect(ui.edit_launch, SIGNAL(editingFinished()),
            this, SLOT(updateSystemInfo()));
    connect(ui.edit_check, SIGNAL(editingFinished()),
            this, SLOT(updateSystemInfo()));
    connect(ui.edit_qdel, SIGNAL(editingFinished()),
            this, SLOT(updateSystemInfo()));
    connect(ui.cb_remote, SIGNAL(toggled(bool)),
            this, SLOT(updateSystemInfo()));
    connect(ui.edit_gulp, SIGNAL(editingFinished()),
            this, SLOT(updateSystemInfo()));
    connect(ui.edit_host, SIGNAL(editingFinished()),
            this, SLOT(updateSystemInfo()));
    connect(ui.edit_username, SIGNAL(editingFinished()),
            this, SLOT(updateSystemInfo()));
    connect(ui.edit_rempath, SIGNAL(editingFinished()),
            this, SLOT(updateSystemInfo()));
  }

  TabSys::~TabSys()
  {
    //qDebug() << "TabSys::~TabSys() called";
  }

  void TabSys::writeSettings() {
    //qDebug() << "TabSys::writeSettings() called";
    QSettings settings; // Already set up in avogadro/src/main.cpp

    settings.setValue("xtalopt/dialog/sys/file/path",		m_opt->filePath);
    settings.setValue("xtalopt/dialog/sys/file/base",		m_opt->fileBase);
    settings.setValue("xtalopt/dialog/sys/queue/launch",	m_opt->launchCommand);
    settings.setValue("xtalopt/dialog/sys/queue/check",		m_opt->queueCheck);
    settings.setValue("xtalopt/dialog/sys/queue/qdel",		m_opt->queueDelete);
    settings.setValue("xtalopt/dialog/sys/path/gulp",		m_opt->gulpPath);
    settings.setValue("xtalopt/dialog/sys/remote/host",		m_opt->host);
    settings.setValue("xtalopt/dialog/sys/remote/username",	m_opt->username);
    settings.setValue("xtalopt/dialog/sys/remote/rempath",	m_opt->rempath);
    settings.setValue("xtalopt/dialog/using/remote",            m_opt->using_remote);
  }

  void TabSys::readSettings() {
    //qDebug() << "TabSys::readSettings() called";
    QSettings settings; // Already set up in avogadro/src/main.cpp

    ui.edit_path->setText(	settings.value("xtalopt/dialog/sys/file/path",		"/tmp").toString());
    ui.edit_base->setText(	settings.value("xtalopt/dialog/sys/file/base",		"/opt-").toString());
    ui.edit_launch->setText(	settings.value("xtalopt/dialog/sys/queue/launch",	"qsub").toString());
    ui.edit_check->setText(	settings.value("xtalopt/dialog/sys/queue/check",	"qstat").toString());
    ui.edit_qdel->setText(	settings.value("xtalopt/dialog/sys/queue/qdel",		"qdel").toString());
    ui.edit_gulp->setText(	settings.value("xtalopt/dialog/sys/path/gulp",		"").toString());
    ui.edit_host->setText(	settings.value("xtalopt/dialog/sys/remote/host",	"").toString());
    ui.edit_username->setText(	settings.value("xtalopt/dialog/sys/remote/username",	"").toString());
    ui.edit_rempath->setText(	settings.value("xtalopt/dialog/sys/remote/rempath",	"").toString());
    ui.cb_remote->setChecked(	settings.value("xtalopt/dialog/using/remote",		false).toBool());
    updateSystemInfo();
  }

  void TabSys::updateGUI() {
    //qDebug() << "TabSys::updateGUI() called";
    ui.edit_path->setText(	m_opt->filePath);
    ui.edit_base->setText(	m_opt->fileBase);
    ui.edit_launch->setText(	m_opt->launchCommand);
    ui.edit_check->setText(	m_opt->queueCheck);
    ui.edit_qdel->setText(	m_opt->queueDelete);
    ui.edit_gulp->setText(	m_opt->gulpPath);
    ui.edit_host->setText(	m_opt->host);
    ui.edit_username->setText(	m_opt->username);
    ui.edit_rempath->setText(	m_opt->rempath);
    ui.cb_remote->setChecked(	m_opt->using_remote);

    // Hide optType specific settings
    switch (m_opt->optType) {
    case XtalOpt::OptType_VASP:
    case XtalOpt::OptType_PWscf:
      ui.edit_path->setVisible(true);
      ui.edit_base->setVisible(true);
      ui.edit_launch->setVisible(true);
      ui.edit_check->setVisible(true);
      ui.edit_qdel->setVisible(true);
      ui.edit_gulp->setVisible(false);
      ui.cb_remote->setVisible(true);
      ui.label_path->setVisible(true);
      ui.label_base->setVisible(true);
      ui.label_launch->setVisible(true);
      ui.label_check->setVisible(true);
      ui.label_qdel->setVisible(true);
      ui.label_gulp->setVisible(false);
      break;
    case XtalOpt::OptType_GULP:
      ui.edit_path->setVisible(true);
      ui.edit_base->setVisible(true);
      ui.edit_launch->setVisible(false);
      ui.edit_check->setVisible(false);
      ui.edit_qdel->setVisible(false);
      ui.edit_gulp->setVisible(true);
      ui.cb_remote->setVisible(false);
      ui.label_path->setVisible(true);
      ui.label_base->setVisible(true);
      ui.label_launch->setVisible(false);
      ui.label_check->setVisible(false);
      ui.label_qdel->setVisible(false);
      ui.label_gulp->setVisible(true);
      break;
    default:
      qWarning() << "TabSys::updateGUI: Selected OptType out of range? " << m_opt->optType;
      break;
    }
  }
  void TabSys::disconnectGUI() {
    //qDebug() << "TabSys::disconnectGUI() called";
    // Nothing I want to disconnect here!
  }

  void TabSys::lockGUI() {
    //qDebug() << "TabSys::lockGUI() called";
    ui.edit_path->setDisabled(true);
    ui.edit_base->setDisabled(true);
    ui.edit_launch->setDisabled(true);
    ui.edit_check->setDisabled(true);
    ui.edit_qdel->setDisabled(true);
    ui.edit_gulp->setDisabled(true);
    ui.cb_remote->setDisabled(true);
    ui.edit_host->setDisabled(true);
    ui.edit_username->setDisabled(true);
    ui.edit_rempath->setDisabled(true);
  }

  void TabSys::updateSystemInfo() {
    //qDebug() << "TabSys::updateSystemInfo() called";
    m_opt->filePath	= ui.edit_path->text();
    m_opt->fileBase	= ui.edit_base->text();
    m_opt->launchCommand	= ui.edit_launch->text();
    m_opt->queueCheck	= ui.edit_check->text();
    m_opt->queueDelete	= ui.edit_qdel->text();
    m_opt->using_remote	= ui.cb_remote->isChecked();
    m_opt->gulpPath	= ui.edit_gulp->text();
    m_opt->host		= ui.edit_host->text();
    m_opt->username	= ui.edit_username->text();
    m_opt->rempath		= ui.edit_rempath->text();
    emit dataChanged();
  }

}

#include "tab_sys.moc"
