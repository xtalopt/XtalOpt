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
  GNU General Public License for more details.
 ***********************************************************************/

#include <xtalopt/ui/tab_sys.h>

#include <xtalopt/xtalopt.h>
#include <xtalopt/ui/dialog.h>

#include <QSettings>

namespace XtalOpt {

  TabSys::TabSys( XtalOptDialog *parent, XtalOpt *p ) :
    QObject( parent ), m_dialog(parent), m_opt(p)
  {
    //qDebug() << "TabSys::TabSys( " << parent <<  " ) called.";

    m_tab_widget = new QWidget;
    ui.setupUi(m_tab_widget);

    m_dialog = parent;

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
    connect(this, SIGNAL(dataChanged()),
            m_dialog, SLOT(updateGUI()));
    connect(m_dialog, SIGNAL(optTypeChanged()),
            this, SLOT(updateGUI()));

    // System Settings connections
    connect(ui.edit_path, SIGNAL(editingFinished()),
            this, SLOT(updateSystemInfo()));
    connect(ui.edit_description, SIGNAL(editingFinished()),
            this, SLOT(updateSystemInfo()));
    connect(ui.edit_qsub, SIGNAL(editingFinished()),
            this, SLOT(updateSystemInfo()));
    connect(ui.edit_qstat, SIGNAL(editingFinished()),
            this, SLOT(updateSystemInfo()));
    connect(ui.edit_qdel, SIGNAL(editingFinished()),
            this, SLOT(updateSystemInfo()));
    connect(ui.cb_remote, SIGNAL(toggled(bool)),
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

  void TabSys::writeSettings(const QString &filename) {
    SETTINGS(filename);
    settings->beginGroup("xtalopt/sys/");
    settings->setValue("file/path",		m_opt->filePath);
    settings->setValue("description",		m_opt->description);
    settings->setValue("queue/qsub",		m_opt->qsub);
    settings->setValue("queue/qstat",		m_opt->qstat);
    settings->setValue("queue/qdel",		m_opt->qdel);
    settings->setValue("remote/host",		m_opt->host);
    settings->setValue("remote/username",	m_opt->username);
    settings->setValue("remote/rempath",	m_opt->rempath);
    settings->endGroup();

    DESTROY_SETTINGS(filename);
  }

  void TabSys::readSettings(const QString &filename) {
    SETTINGS(filename);
    settings->beginGroup("xtalopt/sys/");
    ui.edit_path->setText(	settings->value("file/path",		"/tmp").toString());
    ui.edit_description->setText(settings->value("description",		"").toString());
    ui.edit_qsub->setText(	settings->value("queue/qsub",		"qsub").toString());
    ui.edit_qstat->setText(	settings->value("queue/qstat",		"qstat").toString());
    ui.edit_qdel->setText(	settings->value("queue/qdel",		"qdel").toString());
    ui.edit_host->setText(	settings->value("remote/host",		"").toString());
    ui.edit_username->setText(	settings->value("remote/username",	"").toString());
    ui.edit_rempath->setText(	settings->value("remote/rempath",	"").toString());
    ui.cb_remote->setChecked(	settings->value("remote",		false).toBool());
    settings->endGroup();

    updateSystemInfo();
  }

  void TabSys::updateGUI() {
    //qDebug() << "TabSys::updateGUI() called";
    ui.edit_path->setText(	m_opt->filePath);
    ui.edit_description->setText(m_opt->description);
    ui.edit_qsub->setText(	m_opt->qsub);
    ui.edit_qstat->setText(	m_opt->qstat);
    ui.edit_qdel->setText(	m_opt->qdel);
    ui.edit_host->setText(	m_opt->host);
    ui.edit_username->setText(	m_opt->username);
    ui.edit_rempath->setText(	m_opt->rempath);

    // Hide optType specific settings
    if (m_opt->optimizer()->getIDString() == "VASP" ||
        m_opt->optimizer()->getIDString() == "PWscf" ) {
      ui.edit_path->setVisible(true);
      ui.edit_description->setVisible(true);
      ui.edit_qsub->setVisible(true);
      ui.edit_qstat->setVisible(true);
      ui.edit_qdel->setVisible(true);
      ui.cb_remote->setVisible(true);
      ui.label_path->setVisible(true);
      ui.label_description->setVisible(true);
      ui.label_launch->setVisible(true);
      ui.label_check->setVisible(true);
      ui.label_qdel->setVisible(true);
    }
    else if ( m_opt->optimizer()->getIDString() == "GULP" ) {
      ui.edit_path->setVisible(true);
      ui.edit_description->setVisible(true);
      ui.edit_qsub->setVisible(false);
      ui.edit_qstat->setVisible(false);
      ui.edit_qdel->setVisible(false);
      ui.cb_remote->setVisible(false);
      ui.label_path->setVisible(true);
      ui.label_description->setVisible(true);
      ui.label_launch->setVisible(false);
      ui.label_check->setVisible(false);
      ui.label_qdel->setVisible(false);
    }
    else {
      qWarning() << "TabSys::updateGUI: Selected OptType unknown? " << m_opt->optimizer()->getIDString();
    }
  }

  void TabSys::disconnectGUI() {
    //qDebug() << "TabSys::disconnectGUI() called";
    // Nothing I want to disconnect here!
  }

  void TabSys::lockGUI() {
    //qDebug() << "TabSys::lockGUI() called";
    ui.edit_path->setDisabled(true);
    ui.edit_description->setDisabled(true);
    ui.edit_qsub->setDisabled(true);
    ui.edit_qstat->setDisabled(true);
    ui.edit_qdel->setDisabled(true);
    ui.cb_remote->setDisabled(true);
    ui.edit_host->setDisabled(true);
    ui.edit_username->setDisabled(true);
    ui.edit_rempath->setDisabled(true);
  }

  void TabSys::updateSystemInfo() {
    //qDebug() << "TabSys::updateSystemInfo() called";
    m_opt->filePath	= ui.edit_path->text();
    m_opt->description	= ui.edit_description->text();
    m_opt->qsub		= ui.edit_qsub->text();
    m_opt->qstat	= ui.edit_qstat->text();
    m_opt->qdel		= ui.edit_qdel->text();
    m_opt->host		= ui.edit_host->text();
    m_opt->username	= ui.edit_username->text();
    m_opt->rempath	= ui.edit_rempath->text();
    emit dataChanged();
  }

}

//#include "tab_sys.moc"
