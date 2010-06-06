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

#include <randomdock/ui/tab_log.h>

#include <randomdock/ui/dialog.h>
#include <randomdock/randomdock.h>
#include <globalsearch/macros.h>

#include <QSettings>
#include <QDateTime>

using namespace std;
using namespace Avogadro;

namespace RandomDock {

  TabLog::TabLog( RandomDockDialog *dialog, RandomDock *opt ) :
    QObject( dialog ),
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

    // Log
    connect(dialog, SIGNAL(newLog(QString)),
            this, SLOT(newLog(QString)));
  }

  TabLog::~TabLog()
  {
    writeSettings();
  }

  void TabLog::writeSettings(const QString &filename)
  {
    SETTINGS(filename);
    settings->beginGroup("randomdock/log");

    settings->endGroup();
    DESTROY_SETTINGS(filename);
  }

  void TabLog::readSettings(const QString &filename)
  {
    SETTINGS(filename);
    settings->beginGroup("randomdock/log");
    settings->endGroup();      
  }

  void TabLog::updateGUI()
  {
  }

  void TabLog::disconnectGUI()
  {
  }

  void TabLog::lockGUI()
  {
  }

  void TabLog::newLog(const QString & info)
  {
    QString entry;
    QString timestamp = QDateTime::currentDateTime().toString("MM/dd/yyyy hh:mm:ss (zzz) -- ");
    
    entry = timestamp + info;
    ui.list_list->addItem(entry);
  }
}

//#include "tab_log.moc"
