/**********************************************************************
  AbstractOptTab - Generic tab for editing templates

  Copyright (C) 2009-2011 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <globalsearch/ui/abstractopttab.h>

#include <globalsearch/bt.h>
#include <globalsearch/macros.h>
#include <globalsearch/searchbase.h>
#include <globalsearch/optimizer.h>
#include <globalsearch/queueinterface.h>
#include <globalsearch/ui/abstractdialog.h>

#include <QComboBox>
#include <QDialog>
#include <QFileDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QTextEdit>

#include <QDebug>

namespace GlobalSearch {

AbstractOptTab::AbstractOptTab(AbstractDialog* parent, SearchBase* p)
  : AbstractTab(parent, p), ui_combo_queueInterfaces(0), ui_combo_optimizers(0),
    ui_combo_templates(0), ui_edit_user1(0), ui_edit_user2(0), ui_edit_user3(0),
    ui_edit_user4(0), ui_list_edit(0), ui_list_optStep(0), ui_push_add(0),
    ui_push_help(0), ui_push_loadScheme(0), ui_push_optimizerConfig(0),
    ui_push_queueInterfaceConfig(0), ui_push_remove(0), ui_push_saveScheme(0),
    ui_edit_opt(0)
{
}

void AbstractOptTab::initialize()
{

  // opt connections
  connect(this, SIGNAL(optimizerChanged(size_t, const std::string&)), m_search,
          SLOT(setOptimizer(size_t, const std::string&)), Qt::DirectConnection);
  connect(this, SIGNAL(queueInterfaceChanged(size_t, const std::string&)),
          m_search, SLOT(setQueueInterface(size_t, const std::string&)),
          Qt::DirectConnection);

  // Dialog connections
  connect(this, SIGNAL(optimizerChanged(size_t, const std::string&)), m_dialog,
          SIGNAL(tabsUpdateGUI()));
  connect(this, SIGNAL(queueInterfaceChanged(size_t, const std::string&)),
          m_dialog, SIGNAL(tabsUpdateGUI()));

  // Edit tab connections
  connect(this, SIGNAL(optimizerChanged(size_t, const std::string&)), this,
          SLOT(populateTemplates()));
  connect(this, SIGNAL(queueInterfaceChanged(size_t, const std::string&)), this,
          SLOT(populateTemplates()));
  connect(this, SIGNAL(optimizerChanged(size_t, const std::string&)), this,
          SLOT(populateOptStepList()));
  connect(this, SIGNAL(queueInterfaceChanged(size_t, const std::string&)), this,
          SLOT(populateOptStepList()));
  connect(ui_push_optimizerConfig, SIGNAL(clicked()), this,
          SLOT(configureOptimizer()));
  connect(ui_push_queueInterfaceConfig, SIGNAL(clicked()), this,
          SLOT(configureQueueInterface()));
  connect(ui_push_help, SIGNAL(clicked()), this, SLOT(showHelp()));
  connect(ui_edit_opt, SIGNAL(textChanged()), this,
          SLOT(saveCurrentTemplate()));
  connect(ui_combo_templates, SIGNAL(currentIndexChanged(int)), this,
          SLOT(updateEditWidget()));
  connect(ui_push_add, SIGNAL(clicked()), this, SLOT(appendOptStep()));
  connect(ui_push_remove, SIGNAL(clicked()), this,
          SLOT(removeCurrentOptStep()));
  connect(ui_list_optStep, SIGNAL(currentRowChanged(int)), this,
          SLOT(populateTemplates()));
  connect(ui_list_optStep, SIGNAL(currentRowChanged(int)), this,
          SLOT(updateGUIQueueInterface()));
  connect(ui_list_optStep, SIGNAL(currentRowChanged(int)), this,
          SLOT(updateGUIOptimizer()));
  connect(ui_list_optStep, SIGNAL(currentRowChanged(int)), this,
          SLOT(updateEditWidget()));
  connect(ui_edit_user1, SIGNAL(editingFinished()), this,
          SLOT(updateUserValues()));
  connect(ui_edit_user2, SIGNAL(editingFinished()), this,
          SLOT(updateUserValues()));
  connect(ui_edit_user3, SIGNAL(editingFinished()), this,
          SLOT(updateUserValues()));
  connect(ui_edit_user4, SIGNAL(editingFinished()), this,
          SLOT(updateUserValues()));
  connect(ui_combo_optimizers, SIGNAL(currentIndexChanged(int)), this,
          SLOT(updateOptimizer()));
  connect(ui_combo_queueInterfaces, SIGNAL(currentIndexChanged(int)), this,
          SLOT(updateQueueInterface()));
  connect(ui_push_saveScheme, SIGNAL(clicked()), this, SLOT(saveScheme()));
  connect(ui_push_loadScheme, SIGNAL(clicked()), this, SLOT(loadScheme()));

  // Populate combo boxes
  unsigned int index;

  //  QueueInterfaces
  ui_combo_queueInterfaces->blockSignals(true);
  ui_combo_queueInterfaces->clear();
  index = 0;
  for (const auto& qiName : m_queueInterfaces)
    ui_combo_queueInterfaces->insertItem(index++, qiName);

  ui_combo_queueInterfaces->blockSignals(false);

  if (index != 0)
    ui_combo_queueInterfaces->setCurrentIndex(0);

  //  Optimizers
  ui_combo_optimizers->blockSignals(true);
  ui_combo_optimizers->clear();
  index = 0;
  for (const auto& optName : m_optimizers)
    ui_combo_optimizers->insertItem(index++, optName);

  ui_combo_optimizers->blockSignals(false);

  if (index != 0)
    ui_combo_optimizers->setCurrentIndex(0);

  AbstractTab::initialize();

  updateGUI();
}

AbstractOptTab::~AbstractOptTab()
{
}

void AbstractOptTab::updateGUIQueueInterface()
{
  if (!m_isInitialized) {
    return;
  }
  Q_ASSERT_X(
    m_queueInterfaces.contains(getCurrentQueueInterface()->getIDString()) ||
      getCurrentQueueInterface() == 0,
    Q_FUNC_INFO, "Current queue interface is unknown to AbstractOptTab.");
  Q_ASSERT(m_queueInterfaces.size() == ui_combo_queueInterfaces->count());

  if (getCurrentQueueInterface()) {
    int qiIndex = m_queueInterfaces.indexOf(
      getCurrentQueueInterface()->getIDString().toLower());
    ui_combo_queueInterfaces->blockSignals(true);
    ui_combo_queueInterfaces->setCurrentIndex(qiIndex);
    ui_combo_queueInterfaces->blockSignals(false);
    if (getCurrentQueueInterface()->hasDialog()) {
      ui_push_queueInterfaceConfig->setEnabled(true);
    } else {
      ui_push_queueInterfaceConfig->setEnabled(false);
    }
  } else {
    ui_push_queueInterfaceConfig->setEnabled(false);
  }
}

void AbstractOptTab::updateGUIOptimizer()
{
  if (!m_isInitialized) {
    return;
  }

  Q_ASSERT_X(m_optimizers.contains(getCurrentOptimizer()->getIDString()) ||
               getCurrentOptimizer() == 0,
             Q_FUNC_INFO, "Current optimizer is unknown to AbstractOptTab.");
  Q_ASSERT(m_optimizers.size() == ui_combo_optimizers->count());

  if (getCurrentOptimizer()) {
    int optIndex =
      m_optimizers.indexOf(getCurrentOptimizer()->getIDString().toLower());
    ui_combo_optimizers->blockSignals(true);
    ui_combo_optimizers->setCurrentIndex(optIndex);
    ui_combo_optimizers->blockSignals(false);
  }
}

void AbstractOptTab::updateGUI()
{
  updateGUIQueueInterface();
  updateGUIOptimizer();

  populateOptStepList();

  ui_edit_user1->setText(m_search->getUser1().c_str());
  ui_edit_user2->setText(m_search->getUser2().c_str());
  ui_edit_user3->setText(m_search->getUser3().c_str());
  ui_edit_user4->setText(m_search->getUser4().c_str());
}

void AbstractOptTab::lockGUI()
{
  ui_combo_optimizers->setDisabled(true);
  ui_combo_queueInterfaces->setDisabled(true);
}

void AbstractOptTab::showHelp()
{
  QDialog dialog(m_dialog);

  QVBoxLayout vLayout(&dialog);
  dialog.setLayout(&vLayout);

  QTextEdit textEdit(&dialog);
  textEdit.setPlainText(m_search->getTemplateKeywordHelp());
  textEdit.setReadOnly(true);

  vLayout.addWidget(&textEdit);

  dialog.resize(QSize(700, 600));
  dialog.exec();
}

void AbstractOptTab::updateQueueInterface()
{
  Q_ASSERT_X(
    m_queueInterfaces.contains(getCurrentQueueInterface()->getIDString()) ||
      getCurrentQueueInterface() == 0,
    Q_FUNC_INFO, "Current queue interface is unknown to AbstractOptTab.");

  unsigned int newQiIndex = ui_combo_queueInterfaces->currentIndex();

  Q_ASSERT(newQiIndex <= m_queueInterfaces.size() - 1);

  QueueInterface* qi = m_search->queueInterface(getCurrentOptStep());

  if (qi->hasDialog()) {
    ui_push_queueInterfaceConfig->setEnabled(true);
  } else {
    ui_push_queueInterfaceConfig->setEnabled(false);
  }

  emit queueInterfaceChanged(getCurrentOptStep(),
                             m_queueInterfaces[newQiIndex].toStdString());
}

void AbstractOptTab::updateOptimizer()
{
  Q_ASSERT_X(m_optimizers.contains(getCurrentOptimizer()->getIDString()) ||
               getCurrentOptimizer() == 0,
             Q_FUNC_INFO, "Current optimizer is unknown to AbstractOptTab.");

  unsigned int newOptimizerIndex = ui_combo_optimizers->currentIndex();

  Q_ASSERT(newOptimizerIndex <= m_optimizers.size() - 1);

  Optimizer* o = m_search->optimizer(getCurrentOptStep());

  if (o->hasDialog()) {
    ui_push_optimizerConfig->setEnabled(true);
  } else {
    ui_push_optimizerConfig->setEnabled(false);
  }

  emit optimizerChanged(getCurrentOptStep(),
                        m_optimizers[newOptimizerIndex].toStdString());
}

void AbstractOptTab::configureQueueInterface()
{
  Q_ASSERT(getCurrentQueueInterface());
  Q_ASSERT(getCurrentQueueInterface()->hasDialog());

  QDialog* d = getCurrentQueueInterface()->dialog();
  Q_ASSERT(d != 0);

  d->show();
  d->exec();
}

void AbstractOptTab::configureOptimizer()
{
  Q_ASSERT(getCurrentOptimizer());
  Q_ASSERT(getCurrentOptimizer()->hasDialog());

  QDialog* d = getCurrentOptimizer()->dialog();
  Q_ASSERT(d != 0);

  d->show();
  d->exec();
}

QStringList AbstractOptTab::getTemplateNames(size_t optStep)
{
  if (!m_isInitialized) {
    return QStringList();
  }
  QStringList templateNames = getCurrentOptimizer()->getTemplateFileNames();
  templateNames.append(getCurrentQueueInterface()->getTemplateFileNames());
  std::sort(templateNames.begin(), templateNames.end());
  return templateNames;
}

void AbstractOptTab::populateTemplates()
{
  if (!m_isInitialized) {
    return;
  }

  QString oldTemplateName = ui_combo_templates->currentText();

  int optStepIndex = getCurrentOptStep();
  ui_combo_templates->blockSignals(true);
  ui_combo_templates->clear();
  ui_combo_templates->insertItems(0, getTemplateNames(optStepIndex));

  // Let's see if the old template name is in the new set of templates,
  // and if it is, set that as the current template
  int newInd = 0;
  for (size_t i = 0; i < ui_combo_templates->count(); ++i) {
    if (ui_combo_templates->itemText(i) == oldTemplateName) {
      newInd = i;
      break;
    }
  }

  ui_combo_templates->blockSignals(false);
  ui_combo_templates->setCurrentIndex(newInd);
}

void AbstractOptTab::updateEditWidget()
{
  if (!m_isInitialized) {
    return;
  }
  int optStepIndex = getCurrentOptStep();
  QStringList filenames = getTemplateNames(optStepIndex);
  int templateInd = ui_combo_templates->currentIndex();
  QString templateName = ui_combo_templates->currentText();
  Q_ASSERT(templateInd >= 0 && templateInd < filenames.size());
  Q_ASSERT(templateName.compare(filenames.at(templateInd)) == 0);

  if (m_search->getNumOptSteps() != ui_list_optStep->count()) {
    populateOptStepList();
  }

  Q_ASSERT(optStepIndex >= 0 && optStepIndex < m_search->getNumOptSteps());

  // Display appropriate entry widget. Only text entry is supported
  // by default, reimplement this function in the derived class if
  // list entry is needed and just call
  // AbstractOptTab::updateEditWidget to handle entries
  // requiring text entry.
  ui_list_edit->setVisible(false);
  ui_edit_opt->setVisible(true);

  // Update text edit widget
  Q_ASSERT(getTemplateNames(optStepIndex).contains(templateName));
  QString text =
    m_search->getTemplate(optStepIndex, templateName.toStdString()).c_str();

  ui_edit_opt->blockSignals(true);
  ui_edit_opt->setText(text);
  ui_edit_opt->blockSignals(false);
}

void AbstractOptTab::saveCurrentTemplate()
{
  int optStepIndex = getCurrentOptStep();
  QStringList filenames = getTemplateNames(optStepIndex);
  int templateInd = ui_combo_templates->currentIndex();
  QString templateName = ui_combo_templates->currentText();
  Q_ASSERT(templateInd >= 0 && templateInd < filenames.size());
  Q_ASSERT(templateName.compare(filenames.at(templateInd)) == 0);

  if (m_search->getNumOptSteps() != ui_list_optStep->count())
    populateOptStepList();

  Q_ASSERT(optStepIndex >= 0 && optStepIndex < m_search->getNumOptSteps());

  // Here we only update from the text edit widget. If any templates
  // are using the list input, reimplement this function in the
  // derived class and handle the lists appropriately.
  QString text = ui_edit_opt->document()->toPlainText();

  m_search->setTemplate(optStepIndex, templateName.toStdString(),
                     text.toStdString());

  bool wasBlocked = ui_edit_opt->blockSignals(true);
  ui_edit_opt->blockSignals(wasBlocked);
}

void AbstractOptTab::updateUserValues()
{
  m_search->setUser1(ui_edit_user1->text().toStdString());
  m_search->setUser2(ui_edit_user2->text().toStdString());
  m_search->setUser3(ui_edit_user3->text().toStdString());
  m_search->setUser4(ui_edit_user4->text().toStdString());
}

void AbstractOptTab::populateOptStepList()
{
  if (!m_isInitialized) {
    return;
  }
  ui_list_optStep->blockSignals(true);

  int currentOptStep = getCurrentOptStep();
  const int maxSteps = m_search->getNumOptSteps();

  if (currentOptStep < 0)
    currentOptStep = 0;
  if (currentOptStep >= maxSteps)
    currentOptStep = maxSteps - 1;

  ui_list_optStep->clear();
  for (int i = 1; i <= maxSteps; ++i) {
    ui_list_optStep->addItem(tr("Optimization %1").arg(i));
  }

  ui_list_optStep->blockSignals(false);
  ui_list_optStep->setCurrentRow(currentOptStep);
}

void AbstractOptTab::appendOptStep()
{
  // Reimplement in derived class if Optimizer generic data is needed
  const int maxSteps = m_search->getNumOptSteps();
  const int currentOptStep = getCurrentOptStep();
  Q_ASSERT(currentOptStep >= 0 && currentOptStep < maxSteps);

  m_search->appendOptStep();

  populateOptStepList();
}

void AbstractOptTab::removeCurrentOptStep()
{
  // Reimplement in derived class if Optimizer generic data is needed
  const int maxSteps = m_search->getNumOptSteps();
  const int currentOptStep = getCurrentOptStep();
  Q_ASSERT(currentOptStep >= 0 && currentOptStep < maxSteps);

  // If this is the last opt step, don't do anything
  if (maxSteps == 1)
    return;

  m_search->removeOptStep(currentOptStep);

  populateOptStepList();
}

void AbstractOptTab::saveScheme()
{
  SETTINGS("");
  QString oldFilename =
    settings->value(m_search->getIDString().toLower() + "/edit/schemePath/", "")
      .toString();
  QString filename = QFileDialog::getSaveFileName(
    nullptr, tr("Save Optimization Scheme as..."), oldFilename,
    "*.scheme;;*.state;;*.*", nullptr, QFileDialog::DontUseNativeDialog);

  // User canceled
  if (filename.isEmpty())
    return;

  settings->setValue(m_search->getIDString().toLower() + "/edit/schemePath/",
                     filename);
  writeSettings(filename);
}

void AbstractOptTab::loadScheme()
{
  SETTINGS("");
  QString oldFilename =
    settings->value(m_search->getIDString().toLower() + "/edit/schemePath/", "")
      .toString();
  QString filename = QFileDialog::getOpenFileName(
    nullptr, tr("Select Optimization Scheme to load..."), oldFilename,
    "*.scheme;;*.state;;*.*", 0, QFileDialog::DontUseNativeDialog);

  // User canceled
  if (filename.isEmpty())
    return;

  settings->setValue(m_search->getIDString().toLower() + "/edit/schemePath/",
                     filename);
  readSettings(filename);
}
}
