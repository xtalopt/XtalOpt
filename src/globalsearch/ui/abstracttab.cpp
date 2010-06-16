/**********************************************************************
  AbstractTab -- Basic GlobalSearch tab functionality

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

#include <globalsearch/ui/abstracttab.h>

#include <globalsearch/ui/abstractdialog.h>
#include <globalsearch/optbase.h>

namespace GlobalSearch {

  AbstractTab::AbstractTab( AbstractDialog *parent,
                            OptBase *p ) :
    QObject( parent ),
    m_dialog(parent),
    m_opt(p)
  {
    m_tab_widget = new QWidget;
  }

  void AbstractTab::initialize()
  {
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
    connect(this, SIGNAL(moleculeChanged(Structure*)),
            m_dialog, SIGNAL(moleculeChanged(Structure*)));
  }

  AbstractTab::~AbstractTab()
  {
  }

}
