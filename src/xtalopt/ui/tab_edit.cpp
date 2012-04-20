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

#include <xtalopt/ui/tab_edit.h>

#include <xtalopt/optimizers/castep.h>
#include <xtalopt/optimizers/gulp.h>
#include <xtalopt/optimizers/mopac.h>
#include <xtalopt/optimizers/openbabeloptimizer.h>
#include <xtalopt/optimizers/pwscf.h>
#include <xtalopt/optimizers/vasp.h>
#include <xtalopt/queueinterfaces/openbabel.h>
#include <xtalopt/ui/dialog.h>
#include <xtalopt/ui/mxtalpreoptconfigdialog.h>
#include <xtalopt/xtalopt.h>

#include <globalsearch/macros.h>
#include <globalsearch/queueinterfaces/internal.h>
#include <globalsearch/queueinterfaces/loadleveler.h>
#include <globalsearch/queueinterfaces/lsf.h>
#include <globalsearch/queueinterfaces/pbs.h>
#include <globalsearch/queueinterfaces/sge.h>
#include <globalsearch/queueinterfaces/slurm.h>

#include <QtCore/QSettings>

#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QFont>
#include <QtGui/QFileDialog>
#include <QtGui/QListWidgetItem>
#include <QtGui/QPushButton>
#include <QtGui/QTextEdit>

using namespace GlobalSearch;

namespace XtalOpt {

  TabEdit::TabEdit( XtalOptDialog *parent, XtalOpt *p ) :
    DefaultEditTab(parent, p)
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    // Fill m_optimizers in order of XtalOpt::OptTypes
    m_optimizers.clear();
    const unsigned int numOptimizers = 6;
    for (unsigned int i = 0; i < numOptimizers; ++i) {
      switch (i) {
      case XtalOpt::OT_VASP:
        m_optimizers.append(new VASPOptimizer (m_opt));
        break;
      case XtalOpt::OT_GULP:
        m_optimizers.append(new GULPOptimizer (m_opt));
        break;
      case XtalOpt::OT_PWscf:
        m_optimizers.append(new PWscfOptimizer (m_opt));
        break;
      case XtalOpt::OT_CASTEP:
        m_optimizers.append(new CASTEPOptimizer (m_opt));
        break;
      case XtalOpt::OT_MOPAC:
        m_optimizers.append(new MopacOptimizer (m_opt));
        break;
      case XtalOpt::OT_OPENBABEL:
        m_optimizers.append(new OpenBabelOptimizer (m_opt));
        break;
      }
    }

    // Fill m_optimizers in order of XtalOpt::QueueInterfaces
    m_queueInterfaces.clear();

#ifdef ENABLE_SSH
    const unsigned int numQIs = 7;
#else
    const unsigned int numQIs = 2;
#endif

    for (unsigned int i = 0; i < numQIs; ++i) {
      switch (i) {
      case XtalOpt::QI_INTERNAL:
        m_queueInterfaces.append(new InternalQueueInterface (m_opt));
        break;
#ifdef ENABLE_SSH
      case XtalOpt::QI_PBS:
        m_queueInterfaces.append(new PbsQueueInterface (m_opt));
        break;
      case XtalOpt::QI_SGE:
        m_queueInterfaces.append(new SgeQueueInterface (m_opt));
        break;
      case XtalOpt::QI_SLURM:
        m_queueInterfaces.append(new SlurmQueueInterface (m_opt));
        break;
      case XtalOpt::QI_LSF:
        m_queueInterfaces.append(new LsfQueueInterface (m_opt));
        break;
      case XtalOpt::QI_LOADLEVELER:
        m_queueInterfaces.append(new LoadLevelerQueueInterface (m_opt));
        break;
        //
        // Don't forget to modify numQIs above, or additions here won't matter!
        //
#endif // ENABLE_SSH
      case XtalOpt::QI_OPENBABEL:
        if (xtalopt->isMolecularXtalSearch())
          m_queueInterfaces.append(new OpenBabelQueueInterface (m_opt));
        break;
      }
    }

    connect(ui_list_edit, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
            this, SLOT(changePOTCAR(QListWidgetItem*)));

    DefaultEditTab::initialize();

    // Preopt stuff is only shown for molecular searches
    this->ui_cb_preopt->setVisible(xtalopt->isMolecularXtalSearch());
    this->ui_push_preoptConfig->setVisible(xtalopt->isMolecularXtalSearch());

    connect(xtalopt, SIGNAL(isMolecularXtalSearchChanged(bool)),
            this->ui_cb_preopt, SLOT(setVisible(bool)));
    connect(xtalopt, SIGNAL(isMolecularXtalSearchChanged(bool)),
            this->ui_push_preoptConfig, SLOT(setVisible(bool)));
    connect(this->ui_cb_preopt, SIGNAL(clicked(bool)),
            this, SLOT(setPreoptimization(bool)));
    connect(this->ui_push_preoptConfig, SIGNAL(clicked()),
            this, SLOT(showPreoptimizationConfigDialog()));

    populateTemplates();
  }

  TabEdit::~TabEdit()
  {
  }

  void TabEdit::updateGUI()
  {
    ui_cb_preopt->blockSignals(true);
    ui_cb_preopt->setChecked(m_opt->usePreopt);
    ui_cb_preopt->blockSignals(false);

    this->AbstractEditTab::updateGUI();
  }

  void TabEdit::writeSettings(const QString &filename)
  {
    SETTINGS(filename);
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    settings->beginGroup("xtalopt/edit");
    const int VERSION = 2;
    settings->setValue("version",          VERSION);

    settings->setValue("description", m_opt->description);
    settings->setValue("localpath", m_opt->filePath);
    settings->setValue("remote/host", m_opt->host);
    settings->setValue("remote/port", m_opt->port);
    settings->setValue("remote/username", m_opt->username);
    settings->setValue("remote/rempath", m_opt->rempath);

    settings->setValue("optimizer", m_opt->optimizer()->getIDString().toLower());
    settings->setValue("queueInterface", m_opt->queueInterface()->getIDString().toLower());

    settings->beginGroup("preopt");
    settings->beginGroup("mxtal");

    settings->setValue("enabled",              xtalopt->usePreopt);
    settings->setValue("econv",                xtalopt->mpo_econv);
    settings->setValue("maxSteps",             xtalopt->mpo_maxSteps);
    settings->setValue("scUpdateInterval",     xtalopt->mpo_sCUpdateInterval);
    settings->setValue("cutoffUpdateInterval", xtalopt->mpo_cutoffUpdateInterval);
    settings->setValue("vdwCut",               xtalopt->mpo_vdwCut);
    settings->setValue("eleCut",               xtalopt->mpo_eleCut);
    settings->setValue("debug",                xtalopt->mpo_debug);

    settings->endGroup(); // mxtal
    settings->endGroup(); // preopt

    settings->endGroup(); // xtalopt/edit
    m_opt->optimizer()->writeSettings(filename);

    DESTROY_SETTINGS(filename);
  }

  void TabEdit::readSettings(const QString &filename) {
    SETTINGS(filename);

    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    settings->beginGroup("xtalopt/edit");
    int loadedVersion = settings->value("version", 0).toInt();

    m_opt->port = settings->value("remote/port", 22).toInt();

    // Temporary variables to test settings. This prevents empty
    // scheme values from overwriting defaults.
    QString tmpstr;

    tmpstr = settings->value("description", "").toString();
    if (!tmpstr.isEmpty()) {
      m_opt->description = tmpstr;
    }

    tmpstr = settings->value("remote/rempath", "").toString();
    if (!tmpstr.isEmpty()) {
      m_opt->rempath = tmpstr;
    }

    tmpstr = settings->value("localpath", "").toString();
    if (!tmpstr.isEmpty()) {
      m_opt->filePath = tmpstr;
    }

    tmpstr = settings->value("remote/host", "").toString();
    if (!tmpstr.isEmpty()) {
      m_opt->host = tmpstr;
    }

    tmpstr = settings->value("remote/username", "").toString();
    if (!tmpstr.isEmpty()) {
      m_opt->username = tmpstr;
    }

    if (loadedVersion >= 2) {
      QString optimizer =
        settings->value("optimizer", "gulp").toString().toLower();
      for (QList<Optimizer*>::const_iterator
             it = m_optimizers.constBegin(),
             it_end = m_optimizers.constEnd();
           it != it_end; ++it) {
        if ((*it)->getIDString().toLower().compare(optimizer) == 0) {
          emit optimizerChanged(*it);
          break;
        }
      }

      QString queueInterface =
        settings->value("queueInterface", "local").toString().toLower();
      for (QList<QueueInterface*>::const_iterator
             it = m_queueInterfaces.constBegin(),
             it_end = m_queueInterfaces.constEnd();
           it != it_end; ++it) {
        if ((*it)->getIDString().toLower().compare(queueInterface) == 0) {
          emit queueInterfaceChanged(*it);
          break;
        }
      }
    }

    settings->beginGroup("preopt");
    settings->beginGroup("mxtal");

    xtalopt->usePreopt                = settings->value("enabled", true).toBool();
    xtalopt->mpo_econv                = settings->value("econv", 1e-4).toDouble();
    xtalopt->mpo_maxSteps             = settings->value("maxSteps", 200).toInt();
    xtalopt->mpo_sCUpdateInterval     = settings->value("scUpdateInterval", 10).toInt();
    xtalopt->mpo_cutoffUpdateInterval = settings->value("cutoffUpdateInterval", -1).toDouble();
    xtalopt->mpo_vdwCut               = settings->value("vdwCut", 7.0).toDouble();
    xtalopt->mpo_eleCut               = settings->value("eleCut", 7.0).toDouble();
    xtalopt->mpo_debug                = settings->value("debug", false).toBool();

    settings->endGroup(); // mxtal
    settings->endGroup(); // preopt

    settings->endGroup(); // xtalopt/edit

    // Update config data
    switch (loadedVersion) {
    case 0:
    case 1: // Renamed optType to optimizer, added
            // queueInterface. Both now use lowercase strings to
            // identify. Took ownership of variables previously held
            // by tabsys.
      {
#ifdef ENABLE_SSH
        // Extract optimizer ID
        ui_combo_optimizers->setCurrentIndex
          (settings->value("xtalopt/edit/optType", 0).toInt());
        // Set QueueInterface based on optimizer
        switch (ui_combo_optimizers->currentIndex()) {
        case XtalOpt::OT_VASP:
          ui_combo_queueInterfaces->setCurrentIndex(XtalOpt::QI_PBS);
          // Copy over job.pbs
          settings->setValue
            ("xtalopt/optimizer/VASP/QI/PBS/job.pbs_list",
             settings->value
             ("xtalopt/optimizer/VASP/job.pbs_list", QStringList("")));
          break;
        case XtalOpt::OT_PWscf:
          ui_combo_queueInterfaces->setCurrentIndex(XtalOpt::QI_PBS);
          // Copy over job.pbs
          settings->setValue
            ("xtalopt/optimizer/PWscf/QI/PBS/job.pbs_list",
             settings->value
             ("xtalopt/optimizer/PWscf/job.pbs_list", QStringList("")));
          break;
        case XtalOpt::OT_CASTEP:
          ui_combo_queueInterfaces->setCurrentIndex(XtalOpt::QI_PBS);
          // Copy over job.pbs
          settings->setValue
            ("xtalopt/optimizer/CASTEP/QI/PBS/job.pbs_list",
             settings->value
             ("xtalopt/optimizer/CASTEP/job.pbs_list", QStringList("")));
          break;
        default:
        case XtalOpt::OT_GULP:
          ui_combo_queueInterfaces->setCurrentIndex(XtalOpt::QI_INTERNAL);
          break;
        }
#endif // ENABLE_SSH

        // Formerly tab_sys stuff. Read from default settings object:
        settings->beginGroup("xtalopt/sys/");
        m_opt->description = settings->value("description", "").toString();
        m_opt->rempath = settings->value("remote/rempath", "").toString();
        m_opt->filePath = settings->value("file/path", "").toString();
        m_opt->host = settings->value("remote/host", "").toString();
        m_opt->port = settings->value("remote/port", 22).toInt();
        m_opt->username = settings->value("remote/username", "").toString();
        m_opt->rempath = settings->value("remote/rempath", "").toString();
        settings->endGroup(); // "xtalopt/sys"
      }
    case 2:
    default:
      break;
    }

    m_opt->optimizer()->readSettings(filename);
    m_opt->queueInterface()->readSettings(filename);

    // Do we need to update the POTCAR info?
    if (ui_combo_optimizers->currentIndex() == XtalOpt::OT_VASP) {
      VASPOptimizer *vopt = qobject_cast<VASPOptimizer*>(m_opt->optimizer());
      if (!vopt->POTCARInfoIsUpToDate(xtalopt->comp.keys())) {
        if (generateVASP_POTCAR_info()) {
          vopt->buildPOTCARs();
        }
      }
    }

    updateGUI();
  }

  void TabEdit::updateEditWidget()
  {
    if (!m_isInitialized) {
      return;
    }

    QStringList filenames = getTemplateNames();
    if (filenames.isEmpty()) {
      ui_list_edit->setVisible(false);
      ui_edit_edit->setVisible(false);

      if (m_opt->optimizer()->getNumberOfOptSteps() !=
          ui_list_optStep->count()) {
        this->populateOptStepList();
      }
      return;
    }

    int templateInd = ui_combo_templates->currentIndex();
    QString templateName = ui_combo_templates->currentText();
    Q_ASSERT(templateInd >= 0 && templateInd < filenames.size());
    Q_ASSERT(templateName.compare(filenames.at(templateInd)) == 0);

    if (m_opt->optimizer()->getIDString().compare("VASP") == 0 &&
        templateName.compare("POTCAR") == 0) {

      if (m_opt->optimizer()->getNumberOfOptSteps() !=
          ui_list_optStep->count()) {
        populateOptStepList();
      }

      int optStepIndex = ui_list_optStep->currentRow();
      Q_ASSERT(optStepIndex >= 0 &&
               optStepIndex < m_opt->optimizer()->getNumberOfOptSteps());

      // Display appropriate entry widget.
      ui_list_edit->setVisible(true);
      ui_edit_edit->setVisible(false);

      XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

      VASPOptimizer *vopt = qobject_cast<VASPOptimizer*>(m_opt->optimizer());
      // Do we need to update the POTCAR info?
      if (!vopt->POTCARInfoIsUpToDate(xtalopt->comp.keys())) {
        if (!generateVASP_POTCAR_info()) {
          return;
        }
        vopt->buildPOTCARs();
      }

      // Build list in GUI
      // "POTCAR info" is of type
      // QList<QHash<QString, QString> >
      // e.g. a list of hashes containing
      // [atomic symbol : pseudopotential file] pairs
      QVariantList potcarInfo = m_opt->optimizer()->getData("POTCAR info").toList();
      QList<QString> symbols = potcarInfo.at(optStepIndex).toHash().keys();
      qSort(symbols);
      ui_list_edit->clear();
      for (int i = 0; i < symbols.size(); i++) {
        ui_list_edit->addItem(tr("%1: %2")
                               .arg(symbols.at(i), 2)
                               .arg(potcarInfo.at(optStepIndex).toHash()[symbols.at(i)].toString()));
      }
    }
    // Default for all templates using text entry
    else {
      AbstractEditTab::updateEditWidget();
    }
  }

  void TabEdit::appendOptStep()
  {
    // Copy the current files into a new entry at the end of the opt step list
    if (m_opt->optimizer()->getIDString() == "VASP" &&
        m_opt->optimizer()->getData("POTCAR info").toList().isEmpty()) {
      QMessageBox::information(m_dialog, "POTCAR info missing!",
                               "You need to specify POTCAR information "
                               "before adding more steps!");
      generateVASP_POTCAR_info();
    }

    // Rebuild POTCARs if needed
    if (m_opt->optimizer()->getIDString() == "VASP") {
      int currentOptStep = ui_list_optStep->currentRow();
      QVariantList potcarInfo = m_opt->optimizer()->getData("POTCAR info").toList();
      potcarInfo.append(potcarInfo.at(currentOptStep));
      m_opt->optimizer()->setData("POTCAR info", potcarInfo);
      qobject_cast<VASPOptimizer*>(m_opt->optimizer())->buildPOTCARs();
    }

    AbstractEditTab::appendOptStep();

    populateOptStepList();
  }

  void TabEdit::removeCurrentOptStep()
  {
    int currentOptStep = ui_list_optStep->currentRow();

    if (VASPOptimizer *vasp = qobject_cast<VASPOptimizer*>
        (m_opt->optimizer())) {
      QList<QVariant> sl = vasp->getData("POTCAR info").toList();
      Q_ASSERT(sl.size() >= currentOptStep + 1);
      sl.removeAt(currentOptStep);
      vasp->setData("POTCAR info", sl);
      vasp->buildPOTCARs();
    }

    GlobalSearch::AbstractEditTab::removeCurrentOptStep();

    populateOptStepList();
  }

  void TabEdit::setPreoptimization(bool b)
  {
    m_opt->usePreopt = b;
  }

  void TabEdit::showPreoptimizationConfigDialog()
  {
    MXtalPreoptConfigDialog dialog (qobject_cast<XtalOpt*>(m_opt), m_opt->dialog());
    dialog.exec();
  }

  void TabEdit::changePOTCAR(QListWidgetItem *item)
  {
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
    // "POTCAR info" is of type
    // QList<QHash<QString, QString> >
    // e.g. a list of hashes containing
    // [atomic symbol : pseudopotential file] pairs
    QVariantList potcarInfo = m_opt->optimizer()->getData("POTCAR info").toList();
    QVariantHash hash = potcarInfo.at(ui_list_optStep->currentRow()).toHash();
    hash.insert(symbol,QVariant(filename));
    potcarInfo.replace(ui_list_optStep->currentRow(), hash);
    m_opt->optimizer()->setData("POTCAR info", potcarInfo);
    qobject_cast<VASPOptimizer*>(m_opt->optimizer())->buildPOTCARs();
    updateEditWidget();
  }

  bool TabEdit::generateVASP_POTCAR_info()
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);
    QSettings settings; // Already set up in avogadro/src/main.cpp
    QString path = settings.value("xtalopt/templates/potcarPath", "").toString();
    QVariantList potcarInfo;

    // Generate list of symbols
    QList<QString> symbols;
    QList<uint> atomicNums = xtalopt->comp.keys();
    qSort(atomicNums);

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
      else {
        // User cancel file selection. Set template selection combo to
        // something else so the list will remain empty and be
        // detected when the search starts. Ref ticket 79.
        int curInd = ui_combo_templates->currentIndex();
        int maxInd = ui_combo_templates->count() - 1;
        int newInd = (curInd == maxInd) ? 0 : maxInd;
        ui_combo_templates->setCurrentIndex(newInd);
        return false;
      }
      hash.insert(symbols.at(i), QVariant(filename));
    }

    for (int i = 0; i < m_opt->optimizer()->getNumberOfOptSteps(); i++) {
      potcarInfo.append(QVariant(hash));
    }

    // Set composition in optimizer
    QVariantList toOpt;
    for (int i = 0; i < atomicNums.size(); i++) {
      toOpt.append(atomicNums.at(i));
    }
    m_opt->optimizer()->setData("Composition", toOpt);

    // Set POTCAR info
    m_opt->optimizer()->setData("POTCAR info", QVariant(potcarInfo));

    updateEditWidget();
    return true;
  }
}
