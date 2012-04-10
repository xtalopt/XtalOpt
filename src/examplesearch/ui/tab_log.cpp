/**********************************************************************
  ExampleSearch -- A tool for analysing a matrix-substrate docking problem

  Copyright (C) 2012 by David C. Lonie

  This library is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as
  published by the Free Software Foundation; either version 2.1 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include <examplesearch/ui/tab_log.h>

#include <examplesearch/ui/dialog.h>
#include <examplesearch/examplesearch.h>
#include <globalsearch/macros.h>

#include <QtCore/QSettings>
#include <QtCore/QDateTime>

using namespace std;
using namespace Avogadro;

namespace ExampleSearch {

  TabLog::TabLog( ExampleSearchDialog *dialog, ExampleSearch *opt ) :
    AbstractTab(dialog, opt)
  {
    ui.setupUi(m_tab_widget);

    // Log
    connect(dialog, SIGNAL(newLog(QString)),
            this, SLOT(newLog(QString)));

    initialize();
  }

  TabLog::~TabLog()
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
