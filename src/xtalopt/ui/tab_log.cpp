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

#include <xtalopt/ui/tab_log.h>

#include <xtalopt/xtalopt.h>
#include <xtalopt/ui/dialog.h>

#include <QSettings>

using namespace std;

namespace Avogadro {

  TabLog::TabLog( XtalOptDialog *parent, XtalOpt *p ) :
    QObject( parent ), m_dialog(parent), m_opt(p)
  {
    //qDebug() << "TabLog::TabLog( " << parent <<  " ) called.";

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

    // Log
    connect(m_dialog, SIGNAL(newLog(QString)),
            this, SLOT(newLog(QString)));
  }

  TabLog::~TabLog()
  {
    //qDebug() << "TabSys::~TabSys() called";
  }

  void TabLog::writeSettings(const QString &) {

  }

  void TabLog::readSettings(const QString &) {

  }

  void TabLog::updateGUI() {
    //qDebug() << "TabLog::updateGUI() called";
    // Nothing to do!
  }

  void TabLog::disconnectGUI() {
    //qDebug() << "TabLog::disconnectGUI() called";
    connect(m_dialog, 0, this, 0);
  }

  void TabLog::lockGUI() {
    //qDebug() << "TabLog::lockGUI() called";
    // Nothing to do!
  }

  void TabLog::newLog(const QString & info) {
    QString entry;
    QString timestamp = QDateTime::currentDateTime().toString("MM/dd/yyyy hh:mm:ss (zzz) -- ");

    entry = timestamp + info;
    ui.list_list->addItem(entry);
  }
}

//#include "tab_log.moc"
