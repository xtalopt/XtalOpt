/**********************************************************************
  XtalOpt - Tools for advanced crystal optimization

  Copyright (C) 2009-2011 by David Lonie

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

#include <QtCore/QSettings>

using namespace std;

namespace XtalOpt {

  TabLog::TabLog( XtalOptDialog *parent, XtalOpt *p ) :
    AbstractTab(parent, p)
  {
    ui.setupUi(m_tab_widget);

    connect(m_dialog, SIGNAL(newLog(QString)),
            this, SLOT(newLog(QString)));

    initialize();
  }

  TabLog::~TabLog()
  {
  }

  void TabLog::disconnectGUI()
  {
    connect(m_dialog, 0, this, 0);
  }

  void TabLog::newLog(const QString & info)
  {
    QString entry;
    QString timestamp = QDateTime::currentDateTime().toString("MM/dd/yyyy hh:mm:ss (zzz) -- ");

    entry = timestamp + info;
    ui.list_list->addItem(entry);
  }
}
