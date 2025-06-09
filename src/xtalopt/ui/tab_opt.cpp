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

#include <xtalopt/ui/tab_opt.h>

#include <xtalopt/optimizers/optimizers.h>
#include <xtalopt/ui/dialog.h>
#include <xtalopt/xtalopt.h>

#include <globalsearch/eleminfo.h>
#include <globalsearch/macros.h>
#include <globalsearch/queueinterface.h>
#include <globalsearch/queueinterfaces/queueinterfaces.h>

#include <QSettings>

#include <QComboBox>
#include <QFileDialog>
#include <QFont>
#include <QListWidgetItem>
#include <QTextEdit>

using namespace GlobalSearch;

namespace XtalOpt {

TabOpt::TabOpt(AbstractDialog* parent, XtalOpt* p) : DefaultOptTab(parent, p)
{
  // Fill m_optimizers in order of XtalOpt::OptTypes
  m_optimizers.clear();
  // FIXME: this is a "mark"! The number of optimizers is hardcoded here!
  const unsigned int numOptimizers = 7;
  for (unsigned int i = 0; i < numOptimizers; ++i) {
    switch (i) {
      case XtalOpt::OT_VASP:
        m_optimizers.append("vasp");
        break;
      case XtalOpt::OT_GULP:
        m_optimizers.append("gulp");
        break;
      case XtalOpt::OT_PWscf:
        m_optimizers.append("pwscf");
        break;
      case XtalOpt::OT_CASTEP:
        m_optimizers.append("castep");
        break;
      case XtalOpt::OT_SIESTA:
        m_optimizers.append("siesta");
        break;
      case XtalOpt::OT_MTP:
        m_optimizers.append("mtp");
        break;
      case XtalOpt::OT_GENERIC:
        m_optimizers.append("generic");
        break;
    }
  }

  // Set the correct index
  if (m_search->optimizer(0)) {
    int optIndex = m_optimizers.indexOf(m_search->optimizer(0)->getIDString());
    ui_combo_optimizers->setCurrentIndex(optIndex);
  }

  // Fill m_optimizers in order of XtalOpt::QueueInterfaces
  m_queueInterfaces.clear();
  const unsigned int numQIs = 6;
  for (unsigned int i = 0; i < numQIs; ++i) {
    switch (i) {
      case XtalOpt::QI_LOCAL:
        m_queueInterfaces.append("local");
        break;
#ifdef ENABLE_SSH
      case XtalOpt::QI_PBS:
        m_queueInterfaces.append("pbs");
        break;
      case XtalOpt::QI_SGE:
        m_queueInterfaces.append("sge");
        break;
      case XtalOpt::QI_SLURM:
        m_queueInterfaces.append("slurm");
        break;
      case XtalOpt::QI_LSF:
        m_queueInterfaces.append("lsf");
        break;
      case XtalOpt::QI_LOADLEVELER:
        m_queueInterfaces.append("loadleveler");
        break;
//
// Don't forget to modify numQIs above, or additions here won't matter!
//
#endif // ENABLE_SSH
    }
  }

  // Set the queue interface index
  if (m_search->queueInterface(0)) {
    int qiIndex =
      m_queueInterfaces.indexOf(m_search->queueInterface(0)->getIDString());
    ui_combo_queueInterfaces->setCurrentIndex(qiIndex);
  }

  connect(ui_list_edit, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this,
          SLOT(changePOTCAR(QListWidgetItem*)));
  connect(ui_list_edit, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this,
          SLOT(changePSF(QListWidgetItem*)));

  DefaultOptTab::initialize();

  populateTemplates();
}

TabOpt::~TabOpt()
{
}

void TabOpt::writeSettings(const QString& filename)
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

  // We have a special function for writing edit settings
  xtalopt->writeEditSettings(filename);
}

void TabOpt::readSettings(const QString& filename)
{
  updateGUI();
}

void TabOpt::loadScheme()
{
  QString filename;

  {
    SETTINGS("");
    QString oldFilename =
      settings->value(m_search->getIDString().toLower() + "/edit/schemePath/", "")
        .toString();
    filename = QFileDialog::getOpenFileName(
      nullptr, tr("Select Optimization Scheme to load..."), oldFilename,
      "*.scheme;;*.state;;*.*", 0, QFileDialog::DontUseNativeDialog);

    // User canceled
    if (filename.isEmpty())
      return;

    settings->setValue(m_search->getIDString().toLower() + "/edit/schemePath/",
                       filename);
  }

  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

  // We have a special function for reading this tab's settings
  xtalopt->readEditSettings(filename);

  updateGUI();
}

void TabOpt::updateEditWidget()
{
  if (!m_isInitialized) {
    return;
  }

  QStringList filenames = getTemplateNames(getCurrentOptStep());
  int templateInd = ui_combo_templates->currentIndex();
  QString templateName = ui_combo_templates->currentText();
  Q_ASSERT(templateInd >= 0 && templateInd < filenames.size());
  Q_ASSERT(templateName.compare(filenames.at(templateInd)) == 0);

  AbstractOptTab::updateEditWidget();
}

void TabOpt::appendOptStep()
{
  AbstractOptTab::appendOptStep();

  populateOptStepList();
}

void TabOpt::removeCurrentOptStep()
{
  AbstractOptTab::removeCurrentOptStep();

  populateOptStepList();
}

void TabOpt::changePOTCAR(QListWidgetItem* item)
{
  // If the optimizer isn't VASP, just return...
  if (getCurrentOptimizer()->getIDString() != "VASP")
    return;

  QSettings settings;

  // Get symbol and filename
  QStringList strl = item->text().split(":");
  QString symbol = strl.at(0).trimmed();

  QStringList files;
  QString path = settings.value("xtalopt/templates/potcarPath", "").toString();
  QString filename = QFileDialog::getOpenFileName(
    nullptr, QString("Select pot file for atom %1").arg(symbol), path,
    QString(), 0, QFileDialog::DontUseNativeDialog);

  // User canceled file selection
  if (filename.isEmpty())
    return;

  QStringList delimited = filename.split("/");
  QString runpath = "";
  // We want to chop off the last item...
  for (size_t i = 0; i < delimited.size() - 1; i++)
    runpath += (delimited[i] + "/");

  // QFileDialog::getOpenFileName() only allows one selection. So we don't
  // have to worry about multiple files.
  settings.setValue("xtalopt/templates/potcarPath", runpath);

  // "POTCAR info" is of type
  // QList<QHash<QString, QString> >
  // e.g. a list of hashes containing
  // [atomic symbol : pseudopotential file] pairs
  QVariantList potcarInfo =
    getCurrentOptimizer()->getData("POTCAR info").toList();
  QVariantHash hash = potcarInfo.at(ui_list_optStep->currentRow()).toHash();
  hash.insert(symbol, QVariant(filename));
  potcarInfo.replace(ui_list_optStep->currentRow(), hash);
  getCurrentOptimizer()->setData("POTCAR info", potcarInfo);
  updateEditWidget();
}

void TabOpt::changePSF(QListWidgetItem* item)
{
  // If the optimizer isn't siesta, just return...
  if (getCurrentOptimizer()->getIDString() != "SIESTA")
    return;

  QSettings settings;

  // Get symbol and filename
  QStringList strl = item->text().split(":");
  QString symbol = strl.at(0).trimmed();

  QString path = settings.value("xtalopt/templates/psfPath", "").toString();
  QString filename = QFileDialog::getOpenFileName(
    nullptr, QString("Select psf file for atom %1").arg(symbol), path,
    QString(), 0, QFileDialog::DontUseNativeDialog);

  // User canceled file selection
  if (filename.isEmpty())
    return;

  QStringList delimited = filename.split("/");
  QString runath = "";
  // We want to chop off the last item on the list
  for (size_t i = 0; i < delimited.size() - 1; i++)
    runath += (delimited[i] + "/");

  settings.setValue("xtalopt/templates/psfPath", runath);

  // "PSF info" is of type
  // QList<QHash<QString, QString> >
  // e.g. a list of hashes containing
  // [atomic symbol : pseudopotential file] pairs
  QVariantList psfInfo = getCurrentOptimizer()->getData("PSF info").toList();
  QVariantHash hash = psfInfo.at(ui_list_optStep->currentRow()).toHash();
  hash.insert(symbol, QVariant(filename));
  psfInfo.replace(ui_list_optStep->currentRow(), hash);
  getCurrentOptimizer()->setData("PSF info", psfInfo);
  updateEditWidget();
}

bool TabOpt::generateVASP_POTCAR_info()
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);
  QSettings settings;
  QString path = settings.value("xtalopt/templates/potcarPath", "").toString();
  QVariantList potcarInfo;

  // Generate list of symbols
  QList<QString> symbols = xtalopt->compList[0].getSymbols();
  std::sort(symbols.begin(), symbols.end());
  QList<uint> atomicNums = xtalopt->compList[0].getAtomicNumbers();
  std::sort(atomicNums.begin(), atomicNums.end());

  QString filename;
  QVariantHash hash;
  for (int i = 0; i < symbols.size(); i++) {
    QString path =
      settings.value("xtalopt/templates/potcarPath", "").toString();
    QString filename = QFileDialog::getOpenFileName(
      nullptr, QString("Select pot file for atom %1").arg(symbols.at(i)), path,
      QString(), 0, QFileDialog::DontUseNativeDialog);

    if (!filename.isEmpty()) {
      QStringList delimited = filename.split("/");
      QString runpath = "";
      // We want to chop off the last item...
      for (size_t i = 0; i < delimited.size() - 1; i++)
        runpath += (delimited[i] + "/");

      settings.setValue("xtalopt/templates/potcarPath", runpath);
    } else {
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

  potcarInfo.append(QVariant(hash));

  // Set composition in optimizer
  QVariantList toOpt;
  for (int i = 0; i < atomicNums.size(); i++) {
    toOpt.append(atomicNums.at(i));
  }
  getCurrentOptimizer()->setData("Composition", toOpt);

  // Set POTCAR info
  getCurrentOptimizer()->setData("POTCAR info", QVariant(potcarInfo));

  updateEditWidget();
  return true;
}

bool TabOpt::generateSIESTA_PSF_info()
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);
  QSettings settings;
  QString path = settings.value("xtalopt/templates/psfPath", "").toString();
  QVariantList psfInfo;

  // Generate list of symbols
  QList<QString> symbols = xtalopt->compList[0].getSymbols();
  std::sort(symbols.begin(), symbols.end());
  QList<uint> atomicNums = xtalopt->compList[0].getAtomicNumbers();
  std::sort(atomicNums.begin(), atomicNums.end());

  QStringList files;
  QVariantHash hash;

  for (int i = 0; i < symbols.size(); i++) {
    QString path = settings.value("xtalopt/templates/psfPath", "").toString();
    QString filename = QFileDialog::getOpenFileName(
      nullptr, QString("Select psf file for atom %1").arg(symbols.at(i)), path,
      QString(), 0, QFileDialog::DontUseNativeDialog);

    if (!filename.isEmpty()) {
      QStringList delimited = filename.split("/");
      QString runpath = "";
      // We want to chop off the last item...
      for (size_t i = 0; i < delimited.size() - 1; i++)
        runpath += (delimited[i] + "/");
      settings.setValue("xtalopt/templates/psfPath", runpath);
    } else {
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

  psfInfo.append(QVariant(hash));

  // Set composition in optimizer
  QVariantList toOpt;
  for (int i = 0; i < atomicNums.size(); i++) {
    toOpt.append(atomicNums.at(i));
  }
  getCurrentOptimizer()->setData("Composition", toOpt);

  // Set POTCAR info
  getCurrentOptimizer()->setData("PSF info", QVariant(psfInfo));

  updateEditWidget();
  return true;
}
}
