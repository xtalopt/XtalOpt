/**********************************************************************
  RandomDock -- A tool for analysing a matrix-substrate docking problem

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

#include <randomdock/ui/tab_edit.h>

#include <randomdock/randomdock.h>
#include <randomdock/ui/dialog.h>
#include <randomdock/optimizers/gamess.h>

#include <globalsearch/macros.h>

#include <QFont>
#include <QSettings>
#include <QFileDialog>

using namespace std;
using namespace Avogadro;

namespace RandomDock {

  TabEdit::TabEdit( RandomDockDialog *parent, RandomDock *p ) :
    AbstractTab(parent, p)
  {
    ui.setupUi(m_tab_widget);

    ui.edit_edit->setCurrentFont(QFont("Courier"));

    // opt connections
    connect(this, SIGNAL(optimizerChanged(Optimizer*)),
            m_opt, SLOT(setOptimizer(Optimizer*)));

    // dialog connections
    connect(this, SIGNAL(optimizerChanged(Optimizer*)),
            m_dialog, SIGNAL(tabsUpdateGUI()));

    // Edit tab connections
    connect(ui.push_help, SIGNAL(clicked()),
            this, SLOT(showHelp()));
    connect(ui.edit_edit, SIGNAL(textChanged()),
            this, SLOT(updateTemplates()));
    connect(ui.combo_template, SIGNAL(currentIndexChanged(int)),
            this, SLOT(templateChanged(int)));
    connect(ui.push_add, SIGNAL(clicked()),
            this, SLOT(appendOptStep()));
    connect(ui.push_remove, SIGNAL(clicked()),
            this, SLOT(removeCurrentOptStep()));
    connect(ui.list_opt, SIGNAL(currentRowChanged(int)),
            this, SLOT(optStepChanged()));
    connect(ui.edit_user1, SIGNAL(editingFinished()),
            this, SLOT(updateUserValues()));
    connect(ui.edit_user2, SIGNAL(editingFinished()),
            this, SLOT(updateUserValues()));
    connect(ui.edit_user3, SIGNAL(editingFinished()),
            this, SLOT(updateUserValues()));
    connect(ui.edit_user4, SIGNAL(editingFinished()),
            this, SLOT(updateUserValues()));
    connect(ui.combo_optType, SIGNAL(currentIndexChanged(int)),
            this, SLOT(updateOptType()));
    connect(ui.push_saveScheme, SIGNAL(clicked()),
            this, SLOT(saveScheme()));
    connect(ui.push_loadScheme, SIGNAL(clicked()),
            this, SLOT(loadScheme()));
    ui.combo_optType->setCurrentIndex(0);

    initialize();
  }

  TabEdit::~TabEdit()
  {
  }

  void TabEdit::writeSettings(const QString &filename)
  {
    SETTINGS(filename);

    settings->beginGroup("randomdock/edit");
    const int VERSION = 1;
    settings->setValue("version",     VERSION);

    settings->setValue("optType", ui.combo_optType->currentIndex());
    settings->endGroup();
    m_opt->optimizer()->writeSettings(filename);

    DESTROY_SETTINGS(filename);
  }

  void TabEdit::readSettings(const QString &filename)
  {
    SETTINGS(filename);

    settings->beginGroup("randomdock/edit");
    int loadedVersion = settings->value("version", 0).toInt();

    ui.combo_optType->setCurrentIndex( settings->value("optType", 0).toInt());
    settings->endGroup();

    // Update config data
    switch (loadedVersion) {
    case 0:
    case 1:
    default:
      break;
    }

    updateOptType();
    m_opt->optimizer()->readSettings(filename);

    updateGUI();
  }

  void TabEdit::updateGUI()
  {
    populateOptList();
    if (m_opt->optimizer()->getIDString() == "GAMESS") {
      ui.combo_optType->setCurrentIndex(RandomDock::OT_GAMESS);
    }

    templateChanged(ui.combo_template->currentIndex());
    ui.edit_user1->setText(	m_opt->optimizer()->getUser1());
    ui.edit_user2->setText(	m_opt->optimizer()->getUser2());
    ui.edit_user3->setText(	m_opt->optimizer()->getUser3());
    ui.edit_user4->setText(	m_opt->optimizer()->getUser4());
  }

  void TabEdit::lockGUI()
  {
    ui.combo_optType->setDisabled(true);
  }

  void TabEdit::showHelp() {
    QMessageBox::information(m_dialog,
                             "Template Help",
                             m_opt->getTemplateKeywordHelp());
  }

  void TabEdit::updateOptType()
  {
    // Check if the opttype has actually changed and that the
    // optimizer is set.
    if ( m_opt->optimizer()
         && (
             ( ui.combo_optType->currentIndex() == RandomDock::OT_GAMESS
               && m_opt->optimizer()->getIDString() == "GAMESS" )
             )
         ) {
      return;
    }

    ui.combo_template->blockSignals(true);
    ui.combo_template->clear();
    ui.combo_template->blockSignals(false);

    switch (ui.combo_optType->currentIndex()) {
    case RandomDock::OT_GAMESS: {
      // Set total number of templates (2, length of GAMESS_Templates)
      QStringList sl;
      sl << "" << "";
      ui.combo_template->blockSignals(true);
      ui.combo_template->insertItems(0, sl);

      // Set each template at the appropriate index:
      ui.combo_template->removeItem(GAMT_pbs);
      ui.combo_template->insertItem(GAMT_pbs,	tr("job.pbs"));
      ui.combo_template->removeItem(GAMT_inp);
      ui.combo_template->insertItem(GAMT_inp,	tr("job.inp"));
      ui.combo_template->blockSignals(false);

      emit optimizerChanged(new GAMESSOptimizer (m_opt) );
      ui.combo_template->setCurrentIndex(0);

      break;
    }
    default: // shouldn't happen...
      qWarning() << "TabEdit::updateOptType: Selected OptType out of range?";
      break;
    }
    populateOptList();
    templateChanged(0);
  }

  void TabEdit::templateChanged(int ind)
  {
    ui.edit_edit->setCurrentFont(QFont("Courier"));
    if (ind < 0) {
      return;
    }

    int row = ui.list_opt->currentRow();

    if (m_opt->optimizer()->getNumberOfOptSteps() != ui.list_opt->count())
      populateOptList();

    switch (ui.combo_optType->currentIndex()) {
    case RandomDock::OT_GAMESS: {
      // Hide/show appropriate GUI elements
      ui.list_POTCARs->setVisible(false);
      ui.edit_edit->setVisible(true);

      switch (ind) {
      case GAMT_pbs:
        ui.edit_edit->setText(m_opt->optimizer()->getTemplate("job.pbs", row));
        break;
      case GAMT_inp:
        ui.edit_edit->setText(m_opt->optimizer()->getTemplate("job.inp", row));
        break;
      default: // shouldn't happen...
        qWarning() << "TabEdit::templateChanged: Selected template out of range? " << ind;
        break;
      }
      break;
    }
    default: // shouldn't happen...
      qWarning() << "TabEdit::templateChanged: Selected OptStep out of range? "
                 << ui.combo_optType->currentIndex();
      break;
    }
  }

  void TabEdit::updateTemplates()
  {
    int row = ui.list_opt->currentRow();

    switch (ui.combo_optType->currentIndex()) {
    case RandomDock::OT_GAMESS:
      switch (ui.combo_template->currentIndex()) {
      case GAMT_pbs:
        m_opt->optimizer()->setTemplate("job.pbs", ui.edit_edit->document()->toPlainText(), row);
        break;
      case GAMT_inp:
        m_opt->optimizer()->setTemplate("job.inp", ui.edit_edit->document()->toPlainText(), row);
        break;
      default: // shouldn't happen...
        qWarning() << "TabEdit::updateTemplates: Selected template out of range?";
        break;
      }
      break;

    default: // shouldn't happen...
      qWarning() << "TabEdit::updateTemplates: Selected OptStep out of range?";
      break;
    }
  }

  void TabEdit::updateUserValues()
  {
    m_opt->optimizer()->setUser1(ui.edit_user1->text());
    m_opt->optimizer()->setUser2(ui.edit_user2->text());
    m_opt->optimizer()->setUser3(ui.edit_user3->text());
    m_opt->optimizer()->setUser4(ui.edit_user4->text());
  }

  void TabEdit::populateOptList()
  {
    int selection = ui.list_opt->currentRow();
    int maxSteps = m_opt->optimizer()->getNumberOfOptSteps();
    if (selection < 0) selection = 0;
    if (selection >= maxSteps) selection = maxSteps - 1;

    ui.list_opt->blockSignals(true);
    ui.list_opt->clear();
    for (int i = 1; i <= m_opt->optimizer()->getNumberOfOptSteps(); i++) {
      ui.list_opt->addItem(tr("Optimization %1").arg(i));
    }
    ui.list_opt->blockSignals(false);

    ui.list_opt->setCurrentRow(selection);
  }

  void TabEdit::appendOptStep()
  {
    // Copy the current files into a new entry at the end of the opt step list
    int maxSteps = m_opt->optimizer()->getNumberOfOptSteps();
    int currentOptStep = ui.list_opt->currentRow();
    QStringList templates = m_opt->optimizer()->getTemplateNames();
    QString currentTemplate;

    // Add optstep
    for (int i = 0; i < templates.size(); i++) {
      currentTemplate = m_opt->optimizer()->getTemplate(templates.at(i), currentOptStep);
      m_opt->optimizer()->appendTemplate(templates.at(i), currentTemplate);
    }

    populateOptList();
  }

  void TabEdit::removeCurrentOptStep()
  {
    int currentOptStep = ui.list_opt->currentRow();
    int maxSteps = m_opt->optimizer()->getNumberOfOptSteps();
    QStringList templates = m_opt->optimizer()->getTemplateNames();
    QString currentTemplate;
    for (int i = 0; i < templates.size(); i++) {
      m_opt->optimizer()->removeTemplate(templates.at(i), currentOptStep);
    }

    populateOptList();
  }

  void TabEdit::optStepChanged()
  {
    templateChanged(ui.combo_template->currentIndex());
  }

  void TabEdit::saveScheme()
  {
    SETTINGS("");
    QString filename = settings->value("xtalopt/edit/schemePath/", "").toString();
    QFileDialog dialog (NULL, tr("Save Optimization Scheme as..."),
                        filename, "*.scheme;;*.*");
    dialog.selectFile(m_opt->optimizer()->getIDString() + ".scheme");
    dialog.setFileMode(QFileDialog::AnyFile);
    if (dialog.exec())
      filename = dialog.selectedFiles().first();
    else { // User cancel file selection.
      return;
    }
    settings->setValue("xtalopt/edit/schemePath/", filename);
    writeSettings(filename);
  }

  void TabEdit::loadScheme()
  {
    SETTINGS("");
    QString filename = settings->value("xtalopt/edit/schemePath/", "").toString();
    QFileDialog dialog (NULL,
                        tr("Select Optimization Scheme to load..."),
                        filename,
                        "*.scheme;;*.*");
     dialog.setFileMode(QFileDialog::ExistingFile);
    if (dialog.exec())
      filename = dialog.selectedFiles().first();
    else { // User cancel file selection.
      return;
    }
    settings->setValue("xtalopt/edit/schemePath/", filename);
    readSettings(filename);
  }
}
