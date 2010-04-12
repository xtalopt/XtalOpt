/**********************************************************************
  XtalOpt - Tools for advanced crystal optimization

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

#include "../templates.h"

#include <QFont>
#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>

using namespace std;

namespace Avogadro {

  TabEdit::TabEdit( XtalOptDialog *parent, XtalOpt *p ) :
    QObject( parent ), m_dialog(parent), m_opt(p)
  {
    //qDebug() << "TabEdit::TabEdit( " << parent <<  " ) called.";

    m_tab_widget = new QWidget;
    ui.setupUi(m_tab_widget);

    m_dialog = parent;

    ui.edit_edit->setCurrentFont(QFont("Courier"));
    m_opt->optType = XtalOpt::OptType_VASP;

    // dialog connections
    connect(m_dialog, SIGNAL(tabsReadSettings()),
            this, SLOT(readSettings()));
    connect(m_dialog, SIGNAL(tabsWriteSettings()),
            this, SLOT(writeSettings()));
    connect(m_dialog, SIGNAL(tabsUpdateGUI()),
            this, SLOT(updateGUI()));
    connect(m_dialog, SIGNAL(tabsDisconnectGUI()),
            this, SLOT(disconnectGUI()));
    connect(this, SIGNAL(optTypeChanged()),
            m_dialog, SIGNAL(optTypeChanged()));
    connect(m_dialog, SIGNAL(tabsLockGUI()),
            this, SLOT(lockGUI()));

    // Edit tab connections
    connect(ui.push_help, SIGNAL(clicked()),
            this, SLOT(showHelp()));
    connect(ui.edit_edit, SIGNAL(textChanged()),
            this, SLOT(updateTemplates()));
    connect(ui.combo_template, SIGNAL(currentIndexChanged(int)),
            this, SLOT(templateChanged(int)));
    connect(ui.list_POTCARs, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
            this, SLOT(changePOTCAR(QListWidgetItem*)));
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
    connect(this, SIGNAL(optTypeChanged()),
            this, SLOT(updateGUI()));
}

  TabEdit::~TabEdit()
  {
    //qDebug() << "TabEdit::~TabEdit() called";
  }

  void TabEdit::writeSettings() {
    //qDebug() << "TabEdit::writeSettings() called";
    QSettings settings; // Already set up in avogadro/src/main.cpp

    settings.setValue("xtalopt/dialog/optType",				m_opt->optType);
    settings.setValue("xtalopt/dialog/template/VASP/qScript_list",	m_opt->VASP_qScript_list);
    settings.setValue("xtalopt/dialog/template/VASP/KPOINTS_list",	m_opt->VASP_KPOINTS_list);
    settings.setValue("xtalopt/dialog/template/VASP/INCAR_list",	m_opt->VASP_INCAR_list);
    settings.setValue("xtalopt/dialog/template/GULP/gin_list",		m_opt->GULP_gin_list);
    settings.setValue("xtalopt/dialog/template/PWscf/qScript_list",	m_opt->PWscf_qScript_list);
    settings.setValue("xtalopt/dialog/template/PWscf/in_list",		m_opt->PWscf_in_list);
    settings.setValue("xtalopt/dialog/template/user/VASP1",		m_opt->VASPUser1);
    settings.setValue("xtalopt/dialog/template/user/VASP2",		m_opt->VASPUser2);
    settings.setValue("xtalopt/dialog/template/user/VASP3",		m_opt->VASPUser3);
    settings.setValue("xtalopt/dialog/template/user/VASP4",		m_opt->VASPUser4);
    settings.setValue("xtalopt/dialog/template/user/GULP1",		m_opt->GULPUser1);
    settings.setValue("xtalopt/dialog/template/user/GULP2",		m_opt->GULPUser2);
    settings.setValue("xtalopt/dialog/template/user/GULP3",		m_opt->GULPUser3);
    settings.setValue("xtalopt/dialog/template/user/GULP4",		m_opt->GULPUser4);
    settings.setValue("xtalopt/dialog/template/user/PWscf1",		m_opt->PWscfUser1);
    settings.setValue("xtalopt/dialog/template/user/PWscf2",		m_opt->PWscfUser2);
    settings.setValue("xtalopt/dialog/template/user/PWscf3",		m_opt->PWscfUser3);
    settings.setValue("xtalopt/dialog/template/user/PWscf4",		m_opt->PWscfUser4);

  }

  void TabEdit::readSettings() {
    //qDebug() << "TabEdit::readSettings() called";
    QSettings settings; // Already set up in avogadro/src/main.cpp

    m_opt->VASP_qScript_list    =	settings.value("xtalopt/dialog/template/VASP/qScript_list","").toStringList();
    m_opt->VASP_KPOINTS_list    =	settings.value("xtalopt/dialog/template/VASP/KPOINTS_list",
                                                       QStringList(XtalOptTemplate::input_VASP_KPOINTS())).toStringList();
    m_opt->VASP_INCAR_list	=	settings.value("xtalopt/dialog/template/VASP/INCAR_list",
                                                       QStringList(XtalOptTemplate::input_VASP_INCAR())).toStringList();
    m_opt->GULP_gin_list        =	settings.value("xtalopt/dialog/template/GULP/gin_list","").toStringList();
    m_opt->PWscf_qScript_list   =	settings.value("xtalopt/dialog/template/PWscf/qScript_list","").toStringList();
    m_opt->PWscf_in_list        =	settings.value("xtalopt/dialog/template/PWscf/in_list","").toStringList();
    m_opt->VASPUser1 =	settings.value("xtalopt/dialog/template/user/VASP1","").toString();
    m_opt->VASPUser2 =	settings.value("xtalopt/dialog/template/user/VASP2","").toString();
    m_opt->VASPUser3 =	settings.value("xtalopt/dialog/template/user/VASP3","").toString();
    m_opt->VASPUser4 =	settings.value("xtalopt/dialog/template/user/VASP4","").toString();
    m_opt->GULPUser1 =	settings.value("xtalopt/dialog/template/user/GULP1","").toString();
    m_opt->GULPUser2 =	settings.value("xtalopt/dialog/template/user/GULP2","").toString();
    m_opt->GULPUser3 =	settings.value("xtalopt/dialog/template/user/GULP3","").toString();
    m_opt->GULPUser4 =	settings.value("xtalopt/dialog/template/user/GULP4","").toString();
    m_opt->PWscfUser1 =	settings.value("xtalopt/dialog/template/user/PWscf1","").toString();
    m_opt->PWscfUser2 =	settings.value("xtalopt/dialog/template/user/PWscf2","").toString();
    m_opt->PWscfUser3 =	settings.value("xtalopt/dialog/template/user/PWscf3","").toString();
    m_opt->PWscfUser4 =	settings.value("xtalopt/dialog/template/user/PWscf4","").toString();

    // Read optType, set combobox, then call updateOptType.
    ui.combo_optType->setCurrentIndex(m_opt->optType);
    updateOptType();
    updateGUI();
  }

  void TabEdit::updateGUI() {
    //qDebug() << "TabEdit::updateGUI() called";
    ui.combo_optType->setCurrentIndex(m_opt->optType);
    populateOptList();
    templateChanged(ui.combo_template->currentIndex());
    switch (m_opt->optType) {
    case XtalOpt::OptType_VASP:
      ui.edit_user1->setText(	m_opt->VASPUser1);
      ui.edit_user2->setText(	m_opt->VASPUser2);
      ui.edit_user3->setText(	m_opt->VASPUser3);
      ui.edit_user4->setText(	m_opt->VASPUser4);
      break;
    case XtalOpt::OptType_GULP:
      ui.edit_user1->setText(	m_opt->GULPUser1);
      ui.edit_user2->setText(	m_opt->GULPUser2);
      ui.edit_user3->setText(	m_opt->GULPUser3);
      ui.edit_user4->setText(	m_opt->GULPUser4);
      break;
    case XtalOpt::OptType_PWscf:
      ui.edit_user1->setText(	m_opt->PWscfUser1);
      ui.edit_user2->setText(	m_opt->PWscfUser2);
      ui.edit_user3->setText(	m_opt->PWscfUser3);
      ui.edit_user4->setText(	m_opt->PWscfUser4);
      break;
    }
  }

  void TabEdit::disconnectGUI() {
    //qDebug() << "TabEdit::disconnectGUI() called";
    // nothing I want to disconnect here!
  }

  void TabEdit::lockGUI() {
    //qDebug() << "TabEdit::lockGUI() called";
    ui.combo_optType->setDisabled(true);
  }

  void TabEdit::updateOptType() {
    //qDebug() << "TabEdit::updateOptType() called";

    m_opt->optType = XtalOpt::OptTypes(ui.combo_optType->currentIndex());

    ui.combo_template->blockSignals(true);
    ui.combo_template->clear();
    ui.combo_template->blockSignals(false);

    switch (m_opt->optType) {
    case XtalOpt::OptType_VASP: {
      // Set total number of templates (4, length of VASP_Templates)
      QStringList sl;
      sl << "" << "" << "" << "";
      ui.combo_template->blockSignals(true);
      ui.combo_template->insertItems(0, sl);

      // Set each template at the appropriate index:
      ui.combo_template->removeItem(VT_queueScript);
      ui.combo_template->insertItem(VT_queueScript,	tr("Queue Script"));
      ui.combo_template->removeItem(VT_INCAR);
      ui.combo_template->insertItem(VT_INCAR,		"INCAR");
      ui.combo_template->removeItem(VT_POTCAR);
      ui.combo_template->insertItem(VT_POTCAR,		"POTCAR");
      ui.combo_template->removeItem(VT_KPOINTS);
      ui.combo_template->insertItem(VT_KPOINTS,		"KPOINTS");
      ui.combo_template->blockSignals(false);

      break;
    }
    case XtalOpt::OptType_GULP: {
      // Set total number of templates (1, length of GULP_Templates)
      QStringList sl;
      sl << "";
      ui.combo_template->blockSignals(true);
      ui.combo_template->insertItems(0, sl);

      // Set each template at the appropriate index:
      ui.combo_template->removeItem(GT_gin);
      ui.combo_template->insertItem(GT_gin,	tr("GULP .gin"));
      ui.combo_template->blockSignals(false);

      break;
    }
    case XtalOpt::OptType_PWscf: {
      // Set total number of templates (4, length of VASP_Templates)
      QStringList sl;
      sl << "" << "";
      ui.combo_template->blockSignals(true);
      ui.combo_template->insertItems(0, sl);

      // Set each template at the appropriate index:
      ui.combo_template->removeItem(PWscfT_queueScript);
      ui.combo_template->insertItem(PWscfT_queueScript,	tr("Queue Script"));
      ui.combo_template->removeItem(PWscfT_in);
      ui.combo_template->insertItem(PWscfT_in,		tr("Input file"));
      ui.combo_template->blockSignals(false);

      break;
    }
    default: // shouldn't happen...
      qWarning() << "TabEdit::updateOptType: Selected OptType out of range?";
      break;
    }
    populateOptList();
    templateChanged(0);
    emit optTypeChanged();
  }

  void TabEdit::templateChanged(int ind) {
    //qDebug() << "TabEdit::templateChanged( " << ind << " ) called";
    if (ind < 0) {
      qDebug() << "TabEdit::templateChanged: Not changing template to a negative index.";
      return;
    }
    switch (m_opt->optType) {
    case XtalOpt::OptType_VASP: {
      // Populate opt list if needed
      if (m_opt->VASP_INCAR_list.size() != ui.list_opt->count())
        populateOptList();

      // Hide/show appropriate GUI elements
      if (ind == VT_POTCAR) {
        ui.list_POTCARs->setVisible(true);
        ui.edit_edit->setVisible(false);
      }
      else {
        ui.list_POTCARs->setVisible(false);
        ui.edit_edit->setVisible(true);
      }

      // Set edit data
      int row = ui.list_opt->currentRow();

      switch (ind) {
      case VT_queueScript:
        ui.edit_edit->setText(m_opt->VASP_qScript_list.at(row));
        break;
      case VT_INCAR:
        ui.edit_edit->setText(m_opt->VASP_INCAR_list.at(row));
        break;
      case VT_POTCAR:
        {
          // Do we need to update the POTCAR info?
          QList<uint> atomicNums = m_opt->comp->keys();
          qSort(atomicNums);
          if (m_opt->VASP_POTCAR_info.isEmpty() || // No info at all!
              m_opt->VASP_POTCAR_comp != atomicNums // Composition has changed!
              ) {
            generateVASP_POTCAR_info();
            XtalOptTemplate::buildVASP_POTCAR(m_opt);
          }
          ui.list_POTCARs->clear();
          QList<QString> symbols = m_opt->VASP_POTCAR_info.at(row).keys();
          qSort(symbols);
          for (int i = 0; i < symbols.size(); i++) {
            ui.list_POTCARs->addItem(tr("%1: %2")
                                     .arg(symbols.at(i), 2)
                                     .arg(m_opt->VASP_POTCAR_info.at(row)[symbols.at(i)]));
          }
          break;
        }
      case VT_KPOINTS:
        ui.edit_edit->setText(m_opt->VASP_KPOINTS_list.at(row));
        break;
      default: // shouldn't happen...
        qWarning() << "TabEdit::templateChanged: Selected template out of range? " << ind;
        break;
      }
      break;
    }
    case XtalOpt::OptType_GULP: {
      // Populate opt list if needed
      int row = ui.list_opt->currentRow();

      if (m_opt->GULP_gin_list.size() != ui.list_opt->count())
        populateOptList();

      // Hide/show appropriate GUI elements
      ui.list_POTCARs->setVisible(false);
      ui.edit_edit->setVisible(true);

      // Set edit data
      switch (ind) {
      case GT_gin:
        ui.edit_edit->setText(m_opt->GULP_gin_list.at(row));
        break;
      default: // shouldn't happen...
        qWarning() << "TabEdit::templateChanged: Selected template out of range? " << ind;
        break;
      }
      break;
    }
    case XtalOpt::OptType_PWscf: {
      // Populate opt list if needed
      if (m_opt->PWscf_in_list.size() != ui.list_opt->count())
        populateOptList();

      // Hide/show appropriate GUI elements
      ui.list_POTCARs->setVisible(false);
      ui.edit_edit->setVisible(true);

      // Set edit data
      int row = ui.list_opt->currentRow();

      switch (ind) {
      case PWscfT_queueScript:
        ui.edit_edit->setText(m_opt->PWscf_qScript_list.at(row));
        break;
      case PWscfT_in:
        ui.edit_edit->setText(m_opt->PWscf_in_list.at(row));
        break;
      default: // shouldn't happen...
        qWarning() << "TabEdit::templateChanged: Selected template out of range? " << ind;
        break;
      }
      break;
    }
    default: // shouldn't happen...
      qWarning() << "TabEdit::templateChanged: Selected OptStep out of range? " << m_opt->optType;
      break;
    }
  }

  void TabEdit::updateTemplates() {
    //qDebug() << "TabEdit::updateTemplates() called";
    int row = ui.list_opt->currentRow();

    switch (m_opt->optType) {
    case XtalOpt::OptType_VASP:
      switch (ui.combo_template->currentIndex()) {
      case VT_queueScript:
        m_opt->VASP_qScript_list[row] = ui.edit_edit->document()->toPlainText();
        break;
      case VT_INCAR:
        m_opt->VASP_INCAR_list[row] = ui.edit_edit->document()->toPlainText();
        break;
      case VT_KPOINTS:
        m_opt->VASP_KPOINTS_list[row] = ui.edit_edit->document()->toPlainText();
        break;
      default: // shouldn't happen...
        qWarning() << "TabEdit::updateTemplates: Selected template out of range?";
        break;
      }
      break;

    case XtalOpt::OptType_GULP:
      switch (ui.combo_template->currentIndex()) {
      case GT_gin:
        m_opt->GULP_gin_list[row] = ui.edit_edit->document()->toPlainText();
        break;
      default: // shouldn't happen...
        qWarning() << "TabEdit::updateTemplates: Selected template out of range?";
        break;
      }
      break;

    case XtalOpt::OptType_PWscf:
      switch (ui.combo_template->currentIndex()) {
      case PWscfT_queueScript:
        m_opt->PWscf_qScript_list[row] = ui.edit_edit->document()->toPlainText();
        break;
      case PWscfT_in:
        m_opt->PWscf_in_list[row] = ui.edit_edit->document()->toPlainText();
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

  void TabEdit::updateUserValues() {
    //qDebug() << "TabEdit::updateUserValues() called";
    switch (m_opt->optType) {
    case XtalOpt::OptType_VASP:
      m_opt->VASPUser1	= ui.edit_user1->text();
      m_opt->VASPUser2	= ui.edit_user2->text();
      m_opt->VASPUser3	= ui.edit_user3->text();
      m_opt->VASPUser4	= ui.edit_user4->text();
      break;
    case XtalOpt::OptType_GULP:
      m_opt->GULPUser1	= ui.edit_user1->text();
      m_opt->GULPUser2	= ui.edit_user2->text();
      m_opt->GULPUser3	= ui.edit_user3->text();
      m_opt->GULPUser4	= ui.edit_user4->text();
      break;
    case XtalOpt::OptType_PWscf:
      m_opt->PWscfUser1	= ui.edit_user1->text();
      m_opt->PWscfUser2	= ui.edit_user2->text();
      m_opt->PWscfUser3	= ui.edit_user3->text();
      m_opt->PWscfUser4	= ui.edit_user4->text();
      break;
    }
  }

  void TabEdit::changePOTCAR(QListWidgetItem *item) {
    //qDebug() << "TabEdit::changePOTCAR( " << item << " ) called";

    QSettings settings; // Already set up in avogadro/src/main.cpp

    // Get symbol and filename
    QStringList strl = item->text().split(":");
    QString symbol   = strl.at(0).trimmed();
    QString filename = strl.at(1).trimmed();

    QStringList files;
    QString path = settings.value("xtalopt/templates/potcarPath", "").toString();
    QFileDialog dialog (NULL, QString("Select pot file for atom %1").arg(symbol), path);
    dialog.selectFile(filename);
    dialog.setFileMode(QFileDialog::ExistingFile);
    if (dialog.exec()) {
      files = dialog.selectedFiles();
      if (files.size() != 1) { return;} // Only one file per element
      filename = files.first();
      settings.setValue("xtalopt/templates/potcarPath", dialog.directory().absolutePath());
    }
    else { return;} // User cancel file selection.
    (m_opt->VASP_POTCAR_info[ui.list_opt->currentRow()])[symbol] = filename;
    templateChanged(VT_POTCAR);
  }

  void TabEdit::generateVASP_POTCAR_info() {
    //qDebug() << "TabEdit::generateVASP_POTCAR_info( ) called";
    m_opt->VASP_POTCAR_info.clear();
    QHash<QString, QString> hash;
    m_opt->VASP_POTCAR_info.append(hash);
    XtalOptTemplate::input_VASP_POTCAR(m_opt, 0);
    for (int i = 1; i < ui.list_opt->count(); i++)
      m_opt->VASP_POTCAR_info.append(m_opt->VASP_POTCAR_info.at(0));
  }

  void TabEdit::populateOptList() {
    //qDebug() << "TabEdit::populateOptList( ) called";
    int selection = ui.list_opt->currentRow();
    if (selection < 0) selection = 0;

    switch (m_opt->optType) {
    case XtalOpt::OptType_VASP:
      if (selection >= m_opt->VASP_INCAR_list.size())
        selection = m_opt->VASP_INCAR_list.size()-1;
      // Use the INCAR list as the count of objects.
      ui.list_opt->blockSignals(true);
      ui.list_opt->clear();
      for (int i = 1; i <= m_opt->VASP_INCAR_list.size(); i++) {
        ui.list_opt->addItem(tr("Optimization %1").arg(i));
      }
      ui.list_opt->blockSignals(false);
      break;
    case XtalOpt::OptType_GULP:
      if (selection >= m_opt->GULP_gin_list.size())
        selection = m_opt->GULP_gin_list.size()-1;
      // Use the gin list as the count of objects.
      ui.list_opt->blockSignals(true);
      ui.list_opt->clear();
      for (int i = 1; i <= m_opt->GULP_gin_list.size(); i++) {
        ui.list_opt->addItem(tr("Optimization %1").arg(i));
      }
      ui.list_opt->blockSignals(false);
      break;
    case XtalOpt::OptType_PWscf:
      if (selection >= m_opt->PWscf_in_list.size())
        selection = m_opt->PWscf_in_list.size()-1;
      // Use the "in" list as the count of objects.
      ui.list_opt->blockSignals(true);
      ui.list_opt->clear();
      for (int i = 1; i <= m_opt->PWscf_in_list.size(); i++) {
        ui.list_opt->addItem(tr("Optimization %1").arg(i));
      }
      ui.list_opt->blockSignals(false);
      break;
    default: // shouldn't happen...
      qWarning() << "Selected template out of range?";
      break;
    }
    ui.list_opt->setCurrentRow(selection);
  }

  void TabEdit::appendOptStep() {
    //qDebug() << "TabEdit::appendOptStep() called";
    // Copy the current files into a new entry at the end of the opt step list
    switch (m_opt->optType) {
    case XtalOpt::OptType_VASP:
      m_opt->VASP_INCAR_list
        << m_opt->VASP_INCAR_list.at(ui.list_opt->currentRow());
      m_opt->VASP_KPOINTS_list
        << m_opt->VASP_KPOINTS_list.at(ui.list_opt->currentRow());
      m_opt->VASP_qScript_list
        << m_opt->VASP_qScript_list.at(ui.list_opt->currentRow());
      if (m_opt->VASP_POTCAR_info.isEmpty()) {
        if (!m_opt->comp->isEmpty())
          QMessageBox::information(m_dialog, "POTCAR info missing!", "You need to specify POTCAR information before adding more steps!");
        generateVASP_POTCAR_info();
      }
      m_opt->VASP_POTCAR_info
        << m_opt->VASP_POTCAR_info.at(ui.list_opt->currentRow());
      XtalOptTemplate::buildVASP_POTCAR(m_opt);
      break;
    case XtalOpt::OptType_GULP:
      m_opt->GULP_gin_list
        << m_opt->GULP_gin_list.at(ui.list_opt->currentRow());
      break;
    case XtalOpt::OptType_PWscf:
      m_opt->PWscf_qScript_list
        << m_opt->PWscf_qScript_list.at(ui.list_opt->currentRow());
      m_opt->PWscf_in_list
        << m_opt->PWscf_in_list.at(ui.list_opt->currentRow());
      break;
    default: // shouldn't happen...
      qWarning() << "TabEdit::appendOptStep: Selected OptStep out of range?";
      break;
    }
    populateOptList();
  }

  void TabEdit::removeCurrentOptStep() {
    //qDebug() << "TabEdit::removeCurrentOptStep() called";
    switch (m_opt->optType) {
    case XtalOpt::OptType_VASP:
      m_opt->VASP_INCAR_list.removeAt(ui.list_opt->currentRow());
      m_opt->VASP_qScript_list.removeAt(ui.list_opt->currentRow());
      m_opt->VASP_POTCAR_list.removeAt(ui.list_opt->currentRow());
      m_opt->VASP_KPOINTS_list.removeAt(ui.list_opt->currentRow());
      XtalOptTemplate::buildVASP_POTCAR(m_opt);
      break;
    case XtalOpt::OptType_GULP:
      m_opt->GULP_gin_list.removeAt(ui.list_opt->currentRow());
      break;
    case XtalOpt::OptType_PWscf:
      m_opt->PWscf_qScript_list.removeAt(ui.list_opt->currentRow());
      m_opt->PWscf_in_list.removeAt(ui.list_opt->currentRow());
      break;
    default: // shouldn't happen...
      qWarning() << "TabEdit::removeCurrentOptStep: Selected OptStep out of range?";
      break;
    }
    populateOptList();
  }

  void TabEdit::optStepChanged() {
    //qDebug() << "TabEdit::optStepChanged( ) called";
    templateChanged(ui.combo_template->currentIndex());
  }



}
#include "tab_edit.moc"
