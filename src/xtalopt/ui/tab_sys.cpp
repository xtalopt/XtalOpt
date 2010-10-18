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

#include <QtCore/QSettings>
#include <QtGui/QFileDialog>

namespace XtalOpt {

  TabSys::TabSys( XtalOptDialog *parent, XtalOpt *p ) :
    AbstractTab(parent, p)
  {
    ui.setupUi(m_tab_widget);

    connect(this, SIGNAL(dataChanged()),
            m_dialog, SLOT(updateGUI()));

    // System Settings connections
    connect(ui.edit_path, SIGNAL(editingFinished()),
            this, SLOT(updateSystemInfo()));
    connect(ui.push_path, SIGNAL(clicked()),
            this, SLOT(selectLocalPath()));
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
    connect(ui.spin_port, SIGNAL(valueChanged(int)),
            this, SLOT(updateSystemInfo()));
    connect(ui.edit_username, SIGNAL(editingFinished()),
            this, SLOT(updateSystemInfo()));
    connect(ui.edit_rempath, SIGNAL(editingFinished()),
            this, SLOT(updateSystemInfo()));
    connect(ui.edit_gulpPath, SIGNAL(editingFinished()),
            this, SLOT(updateSystemInfo()));
    connect(ui.push_gulpPath, SIGNAL(clicked()),
            this, SLOT(selectGULPPath()));

    initialize();
  }

  TabSys::~TabSys()
  {
  }

  void TabSys::writeSettings(const QString &filename)
  {
    SETTINGS(filename);
    const int VERSION = 2;
    settings->beginGroup("xtalopt/sys/");
    settings->setValue("version",          VERSION);
    settings->setValue("file/path",        m_opt->filePath);
    settings->setValue("file/gulpPath",    qobject_cast<XtalOpt*>(m_opt)->gulpPath);
    settings->setValue("description",      m_opt->description);
    settings->setValue("queue/qsub",       m_opt->qsub);
    settings->setValue("queue/qstat",      m_opt->qstat);
    settings->setValue("queue/qdel",       m_opt->qdel);
    settings->setValue("remote/host",      m_opt->host);
    settings->setValue("remote/port",      m_opt->port);
    settings->setValue("remote/username",  m_opt->username);
    settings->setValue("remote/rempath",   m_opt->rempath);
    settings->endGroup();

    DESTROY_SETTINGS(filename);
  }

  void TabSys::readSettings(const QString &filename)
  {
    SETTINGS(filename);
    settings->beginGroup("xtalopt/sys/");
    int loadedVersion = settings->value("version", 0).toInt();
    ui.edit_path->setText(      settings->value("file/path",       "/tmp").toString());
    ui.edit_gulpPath->setText(  settings->value("file/gulpPath",   "gulp").toString());
    ui.edit_description->setText(settings->value("description",    "").toString());
    ui.edit_qsub->setText(      settings->value("queue/qsub",      "qsub").toString());
    ui.edit_qstat->setText(     settings->value("queue/qstat",     "qstat").toString());
    ui.edit_qdel->setText(      settings->value("queue/qdel",      "qdel").toString());
    ui.edit_host->setText(      settings->value("remote/host",     "").toString());
    ui.spin_port->setValue(     settings->value("remote/port",     "").toInt());
    ui.edit_username->setText(  settings->value("remote/username", "").toString());
    ui.edit_rempath->setText(   settings->value("remote/rempath",  "").toString());
    ui.cb_remote->setChecked(   settings->value("remote",          false).toBool());
    settings->endGroup();

    // Update config data
    switch (loadedVersion) {
    case 0:
    case 1:
      // Added remote/port to settings
      ui.spin_port->setValue(22);
    case 2:
    default:
      break;
    }

    updateSystemInfo();
  }

  void TabSys::updateGUI()
  {
    ui.edit_path->setText(	m_opt->filePath);
    ui.edit_gulpPath->setText(	qobject_cast<XtalOpt*>(m_opt)->gulpPath);
    ui.edit_description->setText(m_opt->description);
    ui.edit_qsub->setText(	m_opt->qsub);
    ui.edit_qstat->setText(	m_opt->qstat);
    ui.edit_qdel->setText(	m_opt->qdel);
    ui.edit_host->setText(	m_opt->host);
    ui.spin_port->setValue(	m_opt->port);
    ui.edit_username->setText(	m_opt->username);
    ui.edit_rempath->setText(	m_opt->rempath);

    // Hide optType specific settings
    if (m_opt->optimizer()->getIDString() == "VASP" ||
        m_opt->optimizer()->getIDString() == "PWscf"||
        m_opt->optimizer()->getIDString() == "CASTEP" ) {
      ui.edit_path->setVisible(true);
      ui.edit_description->setVisible(true);
      ui.edit_qsub->setVisible(true);
      ui.edit_qstat->setVisible(true);
      ui.edit_qdel->setVisible(true);
      ui.cb_remote->setVisible(true);
      ui.cb_gulp->setVisible(false);
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
      ui.cb_gulp->setVisible(true);
      ui.label_path->setVisible(true);
      ui.label_description->setVisible(true);
      ui.label_launch->setVisible(false);
      ui.label_check->setVisible(false);
      ui.label_qdel->setVisible(false);
    }
    else {
      qWarning() << "TabSys::updateGUI: Selected OptType unknown? "
                 << m_opt->optimizer()->getIDString();
    }
  }

  void TabSys::lockGUI()
  {
    ui.edit_path->setDisabled(true);
    ui.edit_gulpPath->setDisabled(true);
    ui.push_path->setDisabled(true);
    ui.push_gulpPath->setDisabled(true);
    ui.edit_description->setDisabled(true);
    ui.edit_qsub->setDisabled(true);
    ui.edit_qstat->setDisabled(true);
    ui.edit_qdel->setDisabled(true);
    ui.cb_remote->setDisabled(true);
    ui.edit_host->setDisabled(true);
    ui.spin_port->setDisabled(true);
    ui.edit_username->setDisabled(true);
    ui.edit_rempath->setDisabled(true);
  }

  void TabSys::selectLocalPath()
  {
    QString dir = QFileDialog::getExistingDirectory(m_dialog,
                                                    tr("Select directory to store structures:"),
                                                    ui.edit_path->text(),
                                                    QFileDialog::ShowDirsOnly
                                                    | QFileDialog::DontResolveSymlinks);

    if (dir.isEmpty()) return; // user canceled

    ui.edit_path->setText(dir);
    updateSystemInfo();
  }

  void TabSys::selectGULPPath()
  {
    QString path = QFileDialog::getOpenFileName(m_dialog,
                                                tr("Select the GULP executable:"),
                                                ui.edit_gulpPath->text());

    if (path.isEmpty()) return; // user canceled

    ui.edit_gulpPath->setText(path);
    updateSystemInfo();
  }

  void TabSys::updateSystemInfo()
  {
    qobject_cast<XtalOpt*>(m_opt)->gulpPath	= ui.edit_gulpPath->text();
    m_opt->filePath	= ui.edit_path->text();
    m_opt->description	= ui.edit_description->text();
    m_opt->qsub		= ui.edit_qsub->text();
    m_opt->qstat	= ui.edit_qstat->text();
    m_opt->qdel		= ui.edit_qdel->text();
    m_opt->host		= ui.edit_host->text();
    m_opt->port		= ui.spin_port->value();
    m_opt->username	= ui.edit_username->text();
    m_opt->rempath	= ui.edit_rempath->text();
    emit dataChanged();
  }

}
