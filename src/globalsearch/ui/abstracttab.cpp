/**********************************************************************
  AbstractTab -- Basic GlobalSearch tab functionality

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

#include <globalsearch/ui/abstracttab.h>

#include <globalsearch/ui/abstractdialog.h>
#include <globalsearch/optbase.h>

#include <QtGui/QApplication>

#include <QtCore/QThread>

namespace GlobalSearch {

  AbstractTab::AbstractTab( AbstractDialog *parent,
                            OptBase *p ) :
    QObject( parent ),
    m_dialog(parent),
    m_opt(p),
    m_isInitialized(false)
  {
    m_tab_widget = new QWidget;
  }

  void AbstractTab::initialize()
  {
    // dialog connections
    connect(m_dialog, SIGNAL(tabsReadSettingsDirect(const QString &)),
            this, SLOT(readSettings(const QString &)),
            Qt::DirectConnection);
    connect(m_dialog, SIGNAL(tabsReadSettingsBlockingQueued(const QString &)),
            this, SLOT(readSettings(const QString &)),
            Qt::BlockingQueuedConnection);
    connect(m_dialog, SIGNAL(tabsWriteSettingsDirect(const QString &)),
            this, SLOT(writeSettings(const QString &)),
            Qt::DirectConnection);
    connect(m_dialog, SIGNAL(tabsWriteSettingsBlockingQueued(const QString &)),
            this, SLOT(writeSettings(const QString &)),
            Qt::BlockingQueuedConnection);
    connect(m_dialog, SIGNAL(tabsUpdateGUI()),
            this, SLOT(updateGUI()));
    connect(m_dialog, SIGNAL(tabsDisconnectGUI()),
            this, SLOT(disconnectGUI()));
    connect(m_dialog, SIGNAL(tabsLockGUI()),
            this, SLOT(lockGUI()));
    connect(this, SIGNAL(moleculeChanged(GlobalSearch::Structure*)),
            m_dialog, SIGNAL(moleculeChanged(GlobalSearch::Structure*)));
    connect(this, SIGNAL(startingBackgroundProcessing()),
            this, SLOT(setBusyCursor()),
            Qt::QueuedConnection);
    connect(this, SIGNAL(finishedBackgroundProcessing()),
            this, SLOT(clearBusyCursor()),
            Qt::QueuedConnection);

    m_isInitialized = true;
    emit initialized();
  }

  AbstractTab::~AbstractTab()
  {
  }

  void AbstractTab::setBusyCursor()
  {
    Q_ASSERT_X(QThread::currentThread() == qApp->thread(), Q_FUNC_INFO,
               "This function cannot be called from an background thread. "
               "Emit AbstractTab::startingBackgroundProcessing instead.");
    qApp->setOverrideCursor( Qt::WaitCursor );
  }

  void AbstractTab::clearBusyCursor()
  {
    Q_ASSERT_X(QThread::currentThread() == qApp->thread(), Q_FUNC_INFO,
               "This function cannot be called from an background thread. "
               "Emit AbstractTab::finishedBackgroundProcessing instead.");
    qApp->restoreOverrideCursor();
  }


}
