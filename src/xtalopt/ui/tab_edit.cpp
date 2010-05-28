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

#include "tab_edit.h"

#include "dialog.h"
#include "../xtalopt.h"
#include "../optimizers/vasp.h"
#include "../optimizers/gulp.h"
#include "../optimizers/pwscf.h"
#include "../../generic/macros.h"

#include <QFont>
#include <QSettings>
#include <QFileDialog>

using namespace std;

namespace Avogadro {

  TabEdit::TabEdit( XtalOptDialog *parent, XtalOpt *p ) :
    QObject( parent ), m_dialog(parent), m_opt(p)
  {
    m_tab_widget = new QWidget;
    ui.setupUi(m_tab_widget);

    ui.edit_edit->setCurrentFont(QFont("Courier"));

    // opt connections
    connect(this, SIGNAL(optimizerChanged(Optimizer*)),
            m_opt, SLOT(setOptimizer(Optimizer*)));

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
    connect(this, SIGNAL(optimizerChanged(Optimizer*)),
            m_dialog, SIGNAL(tabsUpdateGUI()));

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
    connect(ui.push_saveScheme, SIGNAL(clicked()),
            this, SLOT(saveScheme()));
    connect(ui.push_loadScheme, SIGNAL(clicked()),
            this, SLOT(loadScheme()));
    ui.combo_optType->setCurrentIndex(0);
  }

  TabEdit::~TabEdit()
  {
  }

  void TabEdit::writeSettings(const QString &filename) {
    SETTINGS(filename);

    settings->beginGroup("xtalopt/edit");
    settings->setValue("optType", ui.combo_optType->currentIndex());
    settings->endGroup();
    m_opt->optimizer()->writeSettings(filename);

    DESTROY_SETTINGS(filename);
  }

  void TabEdit::readSettings(const QString &filename) {
    SETTINGS(filename);

    settings->beginGroup("xtalopt/edit");
    ui.combo_optType->setCurrentIndex( settings->value("optType", 0).toInt());
    settings->endGroup();

    updateOptType();
    m_opt->optimizer()->readSettings(filename);

    // Do we need to update the POTCAR info?
    if (ui.combo_optType->currentIndex() == XtalOpt::OT_VASP) {
      VASPOptimizer *vopt = qobject_cast<VASPOptimizer*>(m_opt->optimizer());
      if (!vopt->POTCARInfoIsUpToDate(m_opt->comp.keys())) {
        generateVASP_POTCAR_info();
        vopt->buildPOTCARs();
      }
    }

    updateGUI();
  }

  void TabEdit::updateGUI() {
    populateOptList();
    if (m_opt->optimizer()->getIDString() == "VASP") {
      ui.combo_optType->setCurrentIndex(XtalOpt::OT_VASP);
    }
    else if (m_opt->optimizer()->getIDString() == "GULP") {
      ui.combo_optType->setCurrentIndex(XtalOpt::OT_GULP);
    }
    else if (m_opt->optimizer()->getIDString() == "PWscf") {
      ui.combo_optType->setCurrentIndex(XtalOpt::OT_PWscf);
    }

    templateChanged(ui.combo_template->currentIndex());
    ui.edit_user1->setText(	m_opt->optimizer()->getUser1());
    ui.edit_user2->setText(	m_opt->optimizer()->getUser2());
    ui.edit_user3->setText(	m_opt->optimizer()->getUser3());
    ui.edit_user4->setText(	m_opt->optimizer()->getUser4());
  }

  void TabEdit::disconnectGUI()
  {
    // nothing I want to disconnect here!
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
             ( ui.combo_optType->currentIndex() == XtalOpt::OT_VASP
               && m_opt->optimizer()->getIDString() == "VASP" )
             ||
             ( ui.combo_optType->currentIndex() == XtalOpt::OT_GULP
               && m_opt->optimizer()->getIDString() == "GULP" )
             ||
             ( ui.combo_optType->currentIndex() == XtalOpt::OT_PWscf
               && m_opt->optimizer()->getIDString() == "PWscf" )
             ) 
         ) {
      return;
    }

    ui.combo_template->blockSignals(true);
    ui.combo_template->clear();
    ui.combo_template->blockSignals(false);

    switch (ui.combo_optType->currentIndex()) {
    case XtalOpt::OT_VASP: {
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

      emit optimizerChanged(new VASPOptimizer (m_opt) );
      ui.combo_template->setCurrentIndex(0);

      break;
    }
    case XtalOpt::OT_GULP: {
      // Set total number of templates (1, length of GULP_Templates)
      QStringList sl;
      sl << "";
      ui.combo_template->blockSignals(true);
      ui.combo_template->insertItems(0, sl);

      // Set each template at the appropriate index:
      ui.combo_template->removeItem(GT_gin);
      ui.combo_template->insertItem(GT_gin,	tr("GULP .gin"));
      ui.combo_template->blockSignals(false);

      emit optimizerChanged(new GULPOptimizer (m_opt) );
      ui.combo_template->setCurrentIndex(0);

      break;
    }
    case XtalOpt::OT_PWscf: {
      // Set total number of templates (2, length of PWscf_Templates)
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

      emit optimizerChanged(new PWscfOptimizer (m_opt) );
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

  void TabEdit::templateChanged(int ind) {
    ui.edit_edit->setCurrentFont(QFont("Courier"));
    if (ind < 0) {
      qDebug() << "TabEdit::templateChanged: Not changing template to a negative index.";
      return;
    }

    int row = ui.list_opt->currentRow();

    if (m_opt->optimizer()->getNumberOfOptSteps() != ui.list_opt->count())
      populateOptList();

    switch (ui.combo_optType->currentIndex()) {
    case XtalOpt::OT_VASP: {
      // Hide/show appropriate GUI elements
      if (ind == VT_POTCAR) {
        ui.list_POTCARs->setVisible(true);
        ui.edit_edit->setVisible(false);
      }
      else {
        ui.list_POTCARs->setVisible(false);
        ui.edit_edit->setVisible(true);
      }

      switch (ind) {
      case VT_queueScript:
        ui.edit_edit->setText(m_opt->optimizer()->getTemplate("job.pbs", row));
        break;
      case VT_INCAR:
        ui.edit_edit->setText(m_opt->optimizer()->getTemplate("INCAR", row));
        break;
      case VT_POTCAR:
        {
          VASPOptimizer *vopt = qobject_cast<VASPOptimizer*>(m_opt->optimizer());
          // Do we need to update the POTCAR info?
          if (!vopt->POTCARInfoIsUpToDate(m_opt->comp.keys())) {
            generateVASP_POTCAR_info();
            vopt->buildPOTCARs();
          }

          // Build list in GUI
          ui.list_POTCARs->clear();
          // "POTCAR info" is of type
          // QList<QHash<QString, QString> >
          // e.g. a list of hashes containing 
          // [atomic symbol : pseudopotential file] pairs
          QVariantList potcarInfo = m_opt->optimizer()->getData("POTCAR info").toList();
          QList<QString> symbols = potcarInfo.at(row).toHash().keys();
          qSort(symbols);
          for (int i = 0; i < symbols.size(); i++) {
            ui.list_POTCARs->addItem(tr("%1: %2")
                                     .arg(symbols.at(i), 2)
                                     .arg(potcarInfo.at(row).toHash()[symbols.at(i)].toString()));
          }
          break;
        }
      case VT_KPOINTS:
        ui.edit_edit->setText(m_opt->optimizer()->getTemplate("KPOINTS", row));
        break;
      default: // shouldn't happen...
        qWarning() << "TabEdit::templateChanged: Selected template out of range? " << ind;
        break;
      }
      break;
    }
    case XtalOpt::OT_GULP: {
      // Hide/show appropriate GUI elements
      ui.list_POTCARs->setVisible(false);
      ui.edit_edit->setVisible(true);

      // Set edit data
      switch (ind) {
      case GT_gin:
        ui.edit_edit->setText(m_opt->optimizer()->getTemplate("xtal.gin", row));
        break;
      default: // shouldn't happen...
        qWarning() << "TabEdit::templateChanged: Selected template out of range? " << ind;
        break;
      }
      break;
    }
    case XtalOpt::OT_PWscf: {
      // Hide/show appropriate GUI elements
      ui.list_POTCARs->setVisible(false);
      ui.edit_edit->setVisible(true);

      switch (ind) {
      case PWscfT_queueScript:
        ui.edit_edit->setText(m_opt->optimizer()->getTemplate("job.pbs", row));
        break;
      case PWscfT_in:
        ui.edit_edit->setText(m_opt->optimizer()->getTemplate("xtal.in", row));
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

  void TabEdit::updateTemplates() {
    //qDebug() << "TabEdit::updateTemplates() called";
    int row = ui.list_opt->currentRow();

    switch (ui.combo_optType->currentIndex()) {
    case XtalOpt::OT_VASP:
      switch (ui.combo_template->currentIndex()) {
      case VT_queueScript:
        m_opt->optimizer()->setTemplate("job.pbs", ui.edit_edit->document()->toPlainText(), row);
        break;
      case VT_INCAR:
        m_opt->optimizer()->setTemplate("INCAR", ui.edit_edit->document()->toPlainText(), row);
        break;
      case VT_KPOINTS:
        m_opt->optimizer()->setTemplate("KPOINTS", ui.edit_edit->document()->toPlainText(), row);
        break;
      default: // shouldn't happen...
        qWarning() << "TabEdit::updateTemplates: Selected template out of range?";
        break;
      }
      break;

    case XtalOpt::OT_GULP:
      switch (ui.combo_template->currentIndex()) {
      case GT_gin:
        m_opt->optimizer()->setTemplate("xtal.gin", ui.edit_edit->document()->toPlainText(), row);
        break;
      default: // shouldn't happen...
        qWarning() << "TabEdit::updateTemplates: Selected template out of range?";
        break;
      }
      break;

    case XtalOpt::OT_PWscf:
      switch (ui.combo_template->currentIndex()) {
      case PWscfT_queueScript:
        m_opt->optimizer()->setTemplate("job.pbs", ui.edit_edit->document()->toPlainText(), row);
        break;
      case PWscfT_in:
        m_opt->optimizer()->setTemplate("xtal.in", ui.edit_edit->document()->toPlainText(), row);
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
    m_opt->optimizer()->setUser1(ui.edit_user1->text());
    m_opt->optimizer()->setUser2(ui.edit_user2->text());
    m_opt->optimizer()->setUser3(ui.edit_user3->text());
    m_opt->optimizer()->setUser4(ui.edit_user4->text());
  }

  void TabEdit::changePOTCAR(QListWidgetItem *item) {
    QSettings settings; // Already set up in avogadro/src/main.cpp

    // Get symbol and filename
    QStringList strl = item->text().split(":");
    QString symbol   = strl.at(0).trimmed();
    QString filename = strl.at(1).trimmed();
    qDebug() << filename;

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
    // "POTCAR info" is of type
    // QList<QHash<QString, QString> >
    // e.g. a list of hashes containing 
    // [atomic symbol : pseudopotential file] pairs
    qDebug() << filename;
    QVariantList potcarInfo = m_opt->optimizer()->getData("POTCAR info").toList();
    qDebug() << potcarInfo.at(ui.list_opt->currentRow()).toHash().size();
    QVariantHash hash = potcarInfo.at(ui.list_opt->currentRow()).toHash();
    hash.insert(symbol,QVariant(filename));
    potcarInfo.replace(ui.list_opt->currentRow(), hash);
    m_opt->optimizer()->setData("POTCAR info", potcarInfo);
    qobject_cast<VASPOptimizer*>(m_opt->optimizer())->buildPOTCARs();
    templateChanged(VT_POTCAR);
  }

  void TabEdit::generateVASP_POTCAR_info() {
    QSettings settings; // Already set up in avogadro/src/main.cpp
    QString path = settings.value("xtalopt/templates/potcarPath", "").toString();
    QVariantList potcarInfo;

    // Generate list of symbols
    QList<QString> symbols;
    QList<uint> atomicNums = m_opt->comp.keys();
    qSort(atomicNums);
    QVariantList toOpt;
    for (int i = 0; i < atomicNums.size(); i++) toOpt.append(atomicNums.at(i));
    m_opt->optimizer()->setData("Composition", toOpt);
    for (int i = 0; i < atomicNums.size(); i++)
      symbols.append(OpenBabel::etab.GetSymbol(atomicNums.at(i)));
    qSort(symbols);
    QStringList files;
    QString filename;
    QVariantHash hash;
    for (int i = 0; i < symbols.size(); i++) {
      QString path = settings.value("xtalopt/templates/potcarPath", "").toString();
      QFileDialog dialog (NULL, QString("Select pot file for atom %1").arg(symbols.at(i)), path);
      dialog.selectFile(path + "/" + symbols.at(i));
      dialog.setFileMode(QFileDialog::ExistingFile);
      if (dialog.exec()) {
        files = dialog.selectedFiles();
        if (files.size() != 1) { // Ask again!
          i--;
          continue;
        }
        filename = files.first();
        settings.setValue("xtalopt/templates/potcarPath", dialog.directory().absolutePath());
      }
      else { // User cancel file selection. POTCAR will be blank.
        return;
      }
      hash.insert(symbols.at(i), QVariant(filename));
    }

    for (int i = 0; i < m_opt->optimizer()->getNumberOfOptSteps(); i++)
      potcarInfo.append(QVariant(hash));
        
    m_opt->optimizer()->setData("POTCAR info", QVariant(potcarInfo));
    templateChanged(VT_POTCAR);
  }

  void TabEdit::populateOptList() {
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

  void TabEdit::appendOptStep() {
    // Copy the current files into a new entry at the end of the opt step list

    if (m_opt->optimizer()->getIDString() == "VASP" &&
        m_opt->optimizer()->getData("POTCAR info").toStringList().isEmpty()) {
      QMessageBox::information(m_dialog, "POTCAR info missing!", "You need to specify POTCAR information before adding more steps!");
      generateVASP_POTCAR_info();
    }

    int maxSteps = m_opt->optimizer()->getNumberOfOptSteps();
    int currentOptStep = ui.list_opt->currentRow();
    QStringList templates = m_opt->optimizer()->getTemplateNames();
    QString currentTemplate;

    // Rebuild POTCARs if needed
    if (m_opt->optimizer()->getIDString() == "VASP") {
      QVariantList potcarInfo = m_opt->optimizer()->getData("POTCAR info").toList();
      potcarInfo.append(potcarInfo.at(currentOptStep));
      m_opt->optimizer()->setData("POTCAR info", potcarInfo);
      qobject_cast<VASPOptimizer*>(m_opt->optimizer())->buildPOTCARs();
    }

    // Add optstep
    for (int i = 0; i < templates.size(); i++) {
      currentTemplate = m_opt->optimizer()->getTemplate(templates.at(i), currentOptStep);
      m_opt->optimizer()->appendTemplate(templates.at(i), currentTemplate);
    }

    populateOptList();
  }

  void TabEdit::removeCurrentOptStep() {
    int currentOptStep = ui.list_opt->currentRow();
    int maxSteps = m_opt->optimizer()->getNumberOfOptSteps();
    QStringList templates = m_opt->optimizer()->getTemplateNames();
    QString currentTemplate;
    for (int i = 0; i < templates.size(); i++) {
      m_opt->optimizer()->removeTemplate(templates.at(i), currentOptStep);
    }

    if (m_opt->optimizer()->getIDString() == "VASP") {
      QStringList sl = m_opt->optimizer()->getData("POTCAR info").toStringList();
      sl.removeAt(currentOptStep);
      m_opt->optimizer()->setData("POTCAR info", sl);
      qobject_cast<VASPOptimizer*>(m_opt->optimizer())->buildPOTCARs();
    }

    populateOptList();
  }

  void TabEdit::optStepChanged() {
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
    QFileDialog dialog (NULL, tr("Select Optimization Scheme to load..."),
 filename, "*.scheme;;*.*");
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
//#include "tab_edit.moc"
