/**********************************************************************
  XtalOpt - Tools for advanced crystal optimization

  Copyright (C) 2009-2011 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <xtalopt/ui/tab_edit.h>

#include <xtalopt/optimizers/optimizers.h>
#include <xtalopt/ui/dialog.h>
#include <xtalopt/xtalopt.h>

#include <globalsearch/eleminfo.h>
#include <globalsearch/macros.h>
#include <globalsearch/queueinterfaces/queueinterfaces.h>
#include <globalsearch/queueinterface.h>

#include <QSettings>

#include <QComboBox>
#include <QFont>
#include <QFileDialog>
#include <QListWidgetItem>
#include <QTextEdit>

using namespace GlobalSearch;

namespace XtalOpt {

  TabEdit::TabEdit( AbstractDialog *parent, XtalOpt *p ) :
    DefaultEditTab(parent, p)
  {
    // Fill m_optimizers in order of XtalOpt::OptTypes
    m_optimizers.clear();
    const unsigned int numOptimizers = 5;
    for (unsigned int i = 0; i < numOptimizers; ++i) {
      switch (i) {
      case XtalOpt::OT_VASP:
        m_optimizers.append(p->optimizers()["vasp"].get());
        break;
      case XtalOpt::OT_GULP:
        m_optimizers.append(p->optimizers()["gulp"].get());
        break;
      case XtalOpt::OT_PWscf:
        m_optimizers.append(p->optimizers()["pwscf"].get());
        break;
      case XtalOpt::OT_CASTEP:
        m_optimizers.append(p->optimizers()["castep"].get());
        break;
      case XtalOpt::OT_SIESTA:
        m_optimizers.append(p->optimizers()["siesta"].get());
        break;
     }
    }

    // Set the correct index
    if (m_opt->optimizer()) {
      int optIndex = m_optimizers.indexOf(m_opt->optimizer());
      ui_combo_optimizers->setCurrentIndex(optIndex);
    }

    // Fill m_optimizers in order of XtalOpt::QueueInterfaces
    m_queueInterfaces.clear();
    const unsigned int numQIs = 6;
    for (unsigned int i = 0; i < numQIs; ++i) {
      switch (i) {
      case XtalOpt::QI_LOCAL:
        m_queueInterfaces.append(p->queueInterfaces()["local"].get());
        break;
#ifdef ENABLE_SSH
      case XtalOpt::QI_PBS:
        m_queueInterfaces.append(p->queueInterfaces()["pbs"].get());
        break;
      case XtalOpt::QI_SGE:
        m_queueInterfaces.append(p->queueInterfaces()["sge"].get());
        break;
      case XtalOpt::QI_SLURM:
        m_queueInterfaces.append(p->queueInterfaces()["slurm"].get());
        break;
      case XtalOpt::QI_LSF:
        m_queueInterfaces.append(p->queueInterfaces()["lsf"].get());
        break;
      case XtalOpt::QI_LOADLEVELER:
        m_queueInterfaces.append(p->queueInterfaces()["loadleveler"].get());
        break;
        //
        // Don't forget to modify numQIs above, or additions here won't matter!
        //
#endif // ENABLE_SSH
      }
    }

    // Set the queue interface index
    if (m_opt->queueInterface()) {
      int qiIndex = m_queueInterfaces.indexOf(m_opt->queueInterface());
      ui_combo_queueInterfaces->setCurrentIndex(qiIndex);
    }

    connect(ui_list_edit, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
            this, SLOT(changePOTCAR(QListWidgetItem*)));
    connect(ui_list_edit, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
            this, SLOT(changePSF(QListWidgetItem*)));


    DefaultEditTab::initialize();

    populateTemplates();
  }

  TabEdit::~TabEdit()
  {
  }

  void TabEdit::writeSettings(const QString &filename) {
  }

  void TabEdit::readSettings(const QString &filename)
  {
    updateGUI();

    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    // Do we need to update the POTCAR info?
    if (ui_combo_optimizers->currentIndex() == XtalOpt::OT_VASP &&
        !xtalopt->comp.empty()) {
      VASPOptimizer *vopt = qobject_cast<VASPOptimizer*>(m_opt->optimizer());
      if (!vopt->POTCARInfoIsUpToDate(xtalopt->comp.keys())) {
        if (generateVASP_POTCAR_info()) {
          vopt->buildPOTCARs();
        }
      }
    }

    if (ui_combo_optimizers->currentIndex() == XtalOpt::OT_SIESTA &&
      !xtalopt->comp.empty()) {
      SIESTAOptimizer *sopt = qobject_cast<SIESTAOptimizer*>(m_opt->optimizer());
      if (!sopt->PSFInfoIsUpToDate(xtalopt->comp.keys())) {
        if (generateSIESTA_PSF_info()) {
          sopt->buildPSFs();
        }
      }
    }
    updateGUI();
  }

  void TabEdit::loadScheme()
  {
    QString filename;

    {
      SETTINGS("");
      QString oldFilename = settings->value(m_opt->getIDString().toLower() +
                                           "/edit/schemePath/", "").toString();
      filename = QFileDialog::getOpenFileName(nullptr,
                               tr("Select Optimization Scheme to load..."),
                               oldFilename, "*.scheme;;*.state;;*.*",
                               0, QFileDialog::DontUseNativeDialog);

      // User canceled
      if (filename.isEmpty())
        return;

      settings->setValue(m_opt->getIDString().toLower() +
                         "/edit/schemePath/", filename);
    }

    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    // We have a special function for reading this tab's settings
    xtalopt->readEditSettings(filename);

    updateGUI();
  }

  void TabEdit::updateEditWidget()
  {
    if (!m_isInitialized) {
      return;
    }

    QStringList filenames = getTemplateNames();
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
    } else if (m_opt->optimizer()->getIDString().compare("SIESTA") == 0 &&
        templateName.compare("xtal.psf") == 0) {

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

      SIESTAOptimizer *sopt = qobject_cast<SIESTAOptimizer*>(m_opt->optimizer());
      // Do we need to update the PSFCAR info?
      if (!sopt->PSFInfoIsUpToDate(xtalopt->comp.keys())) {
        if (!generateSIESTA_PSF_info()) {
          return;
        }
        sopt->buildPSFs();
      }

      // Build list in GUI
      // "PSF info" is of type
      // QList<QHash<QString, QString> >
      // e.g. a list of hashes containing
      // [atomic symbol : pseudopotential file] pairs
      QVariantList psfInfo = m_opt->optimizer()->getData("PSF info").toList();
      QList<QString> symbols = psfInfo.at(optStepIndex).toHash().keys();
      qSort(symbols);
      ui_list_edit->clear();
      for (int i = 0; i < symbols.size(); i++) {
        ui_list_edit->addItem(tr("%1: %2")
                               .arg(symbols.at(i), 2)
                               .arg(psfInfo.at(optStepIndex).toHash()[symbols.at(i)].toString()));
      }
    }

    // Default for all templates using text entry
    else {
      AbstractEditTab::updateEditWidget();
    }
  }

  void TabEdit::appendOptStep()
  {
    AbstractEditTab::appendOptStep();

    populateOptStepList();
  }

  void TabEdit::removeCurrentOptStep()
  {
    AbstractEditTab::removeCurrentOptStep();

    populateOptStepList();
  }

  void TabEdit::changePOTCAR(QListWidgetItem *item)
  {
    // If the optimizer isn't VASP, just return...
    if (m_opt->optimizer()->getIDString() != "VASP") return;

    QSettings settings;

    // Get symbol and filename
    QStringList strl = item->text().split(":");
    QString symbol   = strl.at(0).trimmed();

    QStringList files;
    QString path = settings.value("xtalopt/templates/potcarPath", "").toString();
    QString filename = QFileDialog::getOpenFileName(nullptr,
      QString("Select pot file for atom %1").arg(symbol), path, QString(),
      0, QFileDialog::DontUseNativeDialog);

    // User canceled file selection
    if (filename.isEmpty()) return;

    QStringList delimited = filename.split("/");
    QString filePath = "";
    // We want to chop off the last item...
    for (size_t i = 0; i < delimited.size() - 1; i++)
      filePath += (delimited[i] + "/");

    // QFileDialog::getOpenFileName() only allows one selection. So we don't
    // have to worry about multiple files.
    settings.setValue("xtalopt/templates/potcarPath", filePath);

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

  void TabEdit::changePSF(QListWidgetItem *item)
  {
    // If the optimizer isn't siesta, just return...
    if (m_opt->optimizer()->getIDString() != "SIESTA") return;

    QSettings settings;

    // Get symbol and filename
    QStringList strl = item->text().split(":");
    QString symbol   = strl.at(0).trimmed();

    QString path = settings.value("xtalopt/templates/psfPath", "").toString();
    QString filename = QFileDialog::getOpenFileName(nullptr,
      QString("Select psf file for atom %1").arg(symbol), path, QString(),
      0, QFileDialog::DontUseNativeDialog);

    // User canceled file selection
    if (filename.isEmpty()) return;

     QStringList delimited = filename.split("/");
     QString filePath = "";
     // We want to chop off the last item on the list
     for (size_t i = 0; i < delimited.size() - 1; i++)
       filePath += (delimited[i] + "/");

    settings.setValue("xtalopt/templates/psfPath", filePath);

    // "PSF info" is of type
    // QList<QHash<QString, QString> >
    // e.g. a list of hashes containing
    // [atomic symbol : pseudopotential file] pairs
    QVariantList psfInfo = m_opt->optimizer()->getData("PSF info").toList();
    QVariantHash hash = psfInfo.at(ui_list_optStep->currentRow()).toHash();
    hash.insert(symbol,QVariant(filename));
    psfInfo.replace(ui_list_optStep->currentRow(), hash);
    m_opt->optimizer()->setData("PSF info", psfInfo);
    qobject_cast<SIESTAOptimizer*>(m_opt->optimizer())->buildPSFs();
    updateEditWidget();
  }
  bool TabEdit::generateVASP_POTCAR_info()
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);
    QSettings settings;
    QString path = settings.value("xtalopt/templates/potcarPath", "").toString();
    QVariantList potcarInfo;

    // Generate list of symbols
    QList<QString> symbols;
    QList<uint> atomicNums = xtalopt->comp.keys();
    qSort(atomicNums);

    for (int i = 0; i < atomicNums.size(); i++) {
      if (atomicNums.at(i) != 0) {
        symbols.append(ElemInfo::getAtomicSymbol(atomicNums.at(i)).c_str());
      }
    }
    qSort(symbols);
    QString filename;
    QVariantHash hash;
    for (int i = 0; i < symbols.size(); i++) {
      QString path = settings.value("xtalopt/templates/potcarPath", "").toString();
      QString filename = QFileDialog::getOpenFileName(nullptr,
        QString("Select pot file for atom %1").arg(symbols.at(i)), path, QString(),
        0, QFileDialog::DontUseNativeDialog);

      if (!filename.isEmpty()) {
        QStringList delimited = filename.split("/");
        QString filePath = "";
        // We want to chop off the last item...
        for (size_t i = 0; i < delimited.size() - 1; i++)
          filePath += (delimited[i] + "/");

        settings.setValue("xtalopt/templates/potcarPath", filePath);
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

  bool TabEdit::generateSIESTA_PSF_info()
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);
    QSettings settings;
    QString path = settings.value("xtalopt/templates/psfPath", "").toString();
    QVariantList psfInfo;

    // Generate list of symbols
    QList<QString> symbols;
    QList<uint> atomicNums = xtalopt->comp.keys();
    qSort(atomicNums);

    for (int i = 0; i < atomicNums.size(); i++) {
      if (atomicNums.at(i) != 0) {
        symbols.append(ElemInfo::getAtomicSymbol(atomicNums.at(i)).c_str());
      }
    }
     qSort(symbols);
     QStringList files;
     QVariantHash hash;

     for (int i = 0; i < symbols.size(); i++) {
      QString path = settings.value("xtalopt/templates/psfPath", "").toString();
      QString filename = QFileDialog::getOpenFileName(nullptr,
        QString("Select psf file for atom %1").arg(symbols.at(i)), path, QString(),
        0, QFileDialog::DontUseNativeDialog);

      if (!filename.isEmpty()) {
        QStringList delimited = filename.split("/");
        QString filePath = "";
        // We want to chop off the last item...
        for (size_t i = 0; i < delimited.size() - 1; i++)
          filePath += (delimited[i] + "/");
        settings.setValue("xtalopt/templates/psfPath", filePath);
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
      psfInfo.append(QVariant(hash));
    }

    // Set composition in optimizer
    QVariantList toOpt;
    for (int i = 0; i < atomicNums.size(); i++) {
      toOpt.append(atomicNums.at(i));
    }
    m_opt->optimizer()->setData("Composition", toOpt);

    // Set POTCAR info
    m_opt->optimizer()->setData("PSF info", QVariant(psfInfo));

    updateEditWidget();
    return true;

  }
}
