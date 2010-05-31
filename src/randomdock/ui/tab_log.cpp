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

#include "tab_log.h"

#include "../randomdock.h"
#include "dialog.h"

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
    connect(dialog, SIGNAL(tabsReadSettings()),
            this, SLOT(readSettings()));
    connect(dialog, SIGNAL(tabsWriteSettings()),
            this, SLOT(writeSettings()));

    // Log
    connect(dialog, SIGNAL(newLog(QString)),
            this, SLOT(newLog(QString)));
  }

  TabLog::~TabLog()
  {
    writeSettings();
  }

  void TabLog::writeSettings()
  {
  }

  void TabLog::readSettings()
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
