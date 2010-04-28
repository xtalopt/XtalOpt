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

#include "tab_edit.h"

#include "templates.h"
#include "randomdockdialog.h"

#include <QFont>
#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>

using namespace std;

namespace Avogadro {

  TabEdit::TabEdit( RandomDockParams *p ) :
    QObject( p->dialog ), m_params(p)
  {
    qDebug() << "TabEdit::TabEdit( " << p <<  " ) called.";

    m_tab_widget = new QWidget;
    ui.setupUi(m_tab_widget);

    ui.edit_edit->setCurrentFont(QFont("Courier"));

    // dialog connections
    connect(p->dialog, SIGNAL(tabsReadSettings()),
            this, SLOT(readSettings()));
    connect(p->dialog, SIGNAL(tabsWriteSettings()),
            this, SLOT(writeSettings()));

    // Edit tab connections
    connect(ui.push_help, SIGNAL(clicked()),
            this, SLOT(showHelp()));
    connect(ui.edit_edit, SIGNAL(textChanged()),
            this, SLOT(updateTemplates()));
    connect(ui.combo_template, SIGNAL(currentIndexChanged(int)),
            this, SLOT(templateChanged(int)));
  }

  TabEdit::~TabEdit()
  {
    qDebug() << "TabEdit::~TabEdit() called";
    writeSettings();
  }

  void TabEdit::writeSettings() {
    qDebug() << "TabEdit::writeSettings() called";
    QSettings settings; // Already set up in avogadro/src/main.cpp

    settings.setValue("randomdock/templates/GAMESSqueueScript",	m_params->GAMESSqueueScript);
    settings.setValue("randomdock/templates/GAMESSconformerOpt",	m_params->GAMESSconformerOpt);
    settings.setValue("randomdock/templates/GAMESSdockingOpt",	m_params->GAMESSdockingOpt);
  }

  void TabEdit::readSettings() {
    qDebug() << "TabEdit::readSettings() called";
    QSettings settings; // Already set up in avogadro/src/main.cpp

    m_params->GAMESSqueueScript	=	settings.value("randomdock/templates/GAMESSqueueScript","").toString();
    m_params->GAMESSconformerOpt	=	settings.value("randomdock/templates/GAMESSconformerOpt",
                                                       Templates::input_GAMESSconformerOpt()).toString();
    m_params->GAMESSdockingOpt	=	settings.value("randomdock/templates/GAMESSdockingOpt",
                                                       Templates::input_GAMESSdockingOpt()).toString();
    templateChanged(0);
  }

  void TabEdit::templateChanged(int ind) {
    qDebug() << "TabEdit::templateChanged( " << ind << " ) called";
    switch (ind) {
    case GAMESST_QueueScript:
      ui.edit_edit->setText(m_params->GAMESSqueueScript);
      break;
    case GAMESST_ConformerOpt:
      ui.edit_edit->setText(m_params->GAMESSconformerOpt);
      break;
    case GAMESST_DockingOpt:
      ui.edit_edit->setText(m_params->GAMESSdockingOpt);
      break;
    default: // shouldn't happen...
      qWarning() << "Selected template out of range? " << ind;
      return;
    }
  }

  void TabEdit::updateTemplates() {
    qDebug() << "TabEdit::updateTemplates() called";
    switch (ui.combo_template->currentIndex()) {
    case GAMESST_QueueScript:
      m_params->GAMESSqueueScript = ui.edit_edit->document()->toPlainText();
      break;
    case GAMESST_ConformerOpt:
      m_params->GAMESSconformerOpt = ui.edit_edit->document()->toPlainText();
      break;
    case GAMESST_DockingOpt:
      m_params->GAMESSdockingOpt = ui.edit_edit->document()->toPlainText();
      break;
    default: // shouldn't happen...
      qWarning() << "Selected template out of range?";
      break;
    }
  }

}
#include "tab_edit.moc"
