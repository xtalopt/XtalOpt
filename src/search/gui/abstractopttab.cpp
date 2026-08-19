/**********************************************************************
  AbstractOptTab - Generic tab for editing templates

  Copyright (C) 2009-2011 by David Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <search/gui/abstractopttab.h>

#include <common/output.h>
#include <search/gui/config_dialogs.h>
#include <search/gui/queueinterfaces/globalqueueinterfacesettingswidget.h>
#include <search/search.h>
#include <search/optimizer.h>
#include <search/queueinterface.h>
#include <search/gui/abstractdialog.h>

#include <QAction>
#include <QAbstractSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QWidget>

#include <map>

namespace Search {

namespace {
bool parseInputAssetLine(const QString& line, QString& id, QString& file)
{
  const QString trimmed = line.trimmed();
  const int space = trimmed.indexOf(' ');
  if (space <= 0)
    return false;

  id = trimmed.left(space).trimmed();
  file = trimmed.mid(space + 1).trimmed();
  if (file.startsWith("%fileContents:", Qt::CaseInsensitive) && file.endsWith("%")) {
    file = file.mid(QString("%fileContents:").size());
    file.chop(1);
    file = file.trimmed();
  }
  return !id.isEmpty() && !file.isEmpty() && !file.contains(';') && !file.contains('#');
}

void makeConfigurationDialogReadOnly(QDialog* dialog, bool allowRuntimeEdits)
{
  for (auto* edit : dialog->findChildren<QLineEdit*>())
    edit->setReadOnly(true);
  for (auto* edit : dialog->findChildren<QTextEdit*>())
    edit->setReadOnly(true);
  for (auto* edit : dialog->findChildren<QPlainTextEdit*>())
    edit->setReadOnly(true);
  for (auto* combo : dialog->findChildren<QComboBox*>())
    combo->setEnabled(false);
  for (auto* spin : dialog->findChildren<QAbstractSpinBox*>())
    spin->setEnabled(false);
  for (auto* checkbox : dialog->findChildren<QCheckBox*>())
    checkbox->setEnabled(false);
  for (auto* button : dialog->findChildren<QPushButton*>()) {
    if (!qobject_cast<QDialogButtonBox*>(button->parentWidget()))
      button->setEnabled(false);
  }
  for (auto* widget : dialog->findChildren<GlobalQueueInterfaceSettingsWidget*>()) {
    widget->setRuntimeOptionsEditable(allowRuntimeEdits);
  }
}

bool isOptimizerInputAssetName(Optimizer* optimizer, QueueInterface* queueInterface,
                               const QString& name)
{
  if (!optimizer || !optimizer->getOptimizerInputAssetNames().contains(name))
    return false;
  if (optimizer->getOptimizerTemplateFileNames().contains(name))
    return false;
  if (queueInterface && queueInterface->getQueueInterfaceTemplateFileNames().contains(name)) {
    return false;
  }
  return true;
}
} // namespace

AbstractOptTab::AbstractOptTab(AbstractDialog* parent, SearchBase* p)
  : AbstractTab(parent, p), ui_combo_queueInterfaces(0), ui_combo_optimizers(0),
    ui_combo_templates(0), ui_edit_user1(0), ui_edit_user2(0), ui_edit_user3(0),
    ui_edit_user4(0), ui_list_optStep(0), ui_push_add(0),
    ui_push_help(0), ui_push_loadScheme(0), ui_push_optimizerConfig(0),
    ui_push_queueInterfaceConfig(0), ui_push_remove(0), ui_push_saveScheme(0),
    ui_edit_opt(0),
    ui_edit_locpath(0), ui_action_browse_locpath(0),
    ui_edit_description(0), ui_cb_logErrorDirs(0),
    ui_cb_cancelJobAfterTime(0), ui_spin_hoursForCancelJob(0),
    m_configDialogsReadOnly(false), m_helpDialog(nullptr)
{
}

void AbstractOptTab::initialize()
{

  // opt connections
  connect(this, &AbstractOptTab::optimizerChanged,
          m_search, &SearchBase::setOptimizer, Qt::DirectConnection);
  connect(this, &AbstractOptTab::queueInterfaceChanged,
          m_search, &SearchBase::setQueueInterface, Qt::DirectConnection);

  // Dialog connections
  connect(this, &AbstractOptTab::optimizerChanged, m_dialog, &AbstractDialog::tabsUpdateGUI);
  connect(this, &AbstractOptTab::queueInterfaceChanged, m_dialog, &AbstractDialog::tabsUpdateGUI);

  // Edit tab connections
  connect(this, &AbstractOptTab::optimizerChanged, this, &AbstractOptTab::populateTemplates);
  connect(this, &AbstractOptTab::queueInterfaceChanged, this, &AbstractOptTab::populateTemplates);
  connect(this, &AbstractOptTab::optimizerChanged, this, &AbstractOptTab::populateOptStepList);
  connect(this, &AbstractOptTab::queueInterfaceChanged, this, &AbstractOptTab::populateOptStepList);
  connect(ui_push_optimizerConfig,     &QPushButton::clicked,
          this, &AbstractOptTab::configureOptimizer);
  connect(ui_push_queueInterfaceConfig, &QPushButton::clicked,
          this, &AbstractOptTab::configureQueueInterface);
  connect(ui_push_help, &QPushButton::clicked, this, &AbstractOptTab::showHelp);
  connect(ui_edit_opt, &QTextEdit::textChanged, this, &AbstractOptTab::saveCurrentTemplate);
  connect(ui_combo_templates,
          static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          this, &AbstractOptTab::updateEditWidget);
  connect(ui_push_add,    &QPushButton::clicked, this, &AbstractOptTab::appendOptStep);
  connect(ui_push_remove, &QPushButton::clicked, this, &AbstractOptTab::removeCurrentOptStep);
  connect(ui_list_optStep, &QListWidget::currentRowChanged,
          this, &AbstractOptTab::populateTemplates);
  connect(ui_list_optStep, &QListWidget::currentRowChanged,
          this, &AbstractOptTab::updateGUIQueueInterface);
  connect(ui_list_optStep, &QListWidget::currentRowChanged,
          this, &AbstractOptTab::updateGUIOptimizer);
  connect(ui_list_optStep, &QListWidget::currentRowChanged,
          this, &AbstractOptTab::updateEditWidget);
  connect(ui_edit_user1, &QLineEdit::editingFinished, this, &AbstractOptTab::updateUserValues);
  connect(ui_edit_user2, &QLineEdit::editingFinished, this, &AbstractOptTab::updateUserValues);
  connect(ui_edit_user3, &QLineEdit::editingFinished, this, &AbstractOptTab::updateUserValues);
  connect(ui_edit_user4, &QLineEdit::editingFinished, this, &AbstractOptTab::updateUserValues);
  connect(ui_combo_optimizers,
          static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          this, &AbstractOptTab::updateOptimizer);
  connect(ui_combo_queueInterfaces,
          static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          this, &AbstractOptTab::updateQueueInterface);
  connect(ui_push_saveScheme, &QPushButton::clicked, this, &AbstractOptTab::saveScheme);
  connect(ui_push_loadScheme, &QPushButton::clicked, this, &AbstractOptTab::loadScheme);
  connect(ui_action_browse_locpath, &QAction::triggered, this, &AbstractOptTab::browseLocWorkDir);
  connect(ui_edit_locpath, &QLineEdit::editingFinished, this, [this]() {
    m_search->setLocWorkDir(ui_edit_locpath->text().trimmed());
  });
  connect(ui_edit_description, &QLineEdit::editingFinished, this, [this]() {
    m_search->setDescription(ui_edit_description->text().trimmed());
  });
  connect(ui_cb_logErrorDirs, &QCheckBox::toggled, this, [this](bool checked) {
    m_search->setLogErrorDirs(checked);
  });

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
  QueueInterface* currentQueueInterface = getCurrentQueueInterface();
  Q_ASSERT_X(currentQueueInterface == 0 || m_queueInterfaces.contains(
                 currentQueueInterface->getIDString().toLower()), Q_FUNC_INFO,
             "Current queue interface is unknown to AbstractOptTab.");
  Q_ASSERT(m_queueInterfaces.size() == ui_combo_queueInterfaces->count());

  if (currentQueueInterface) {
    int qiIndex = m_queueInterfaces.indexOf(currentQueueInterface->getIDString().toLower());
    ui_combo_queueInterfaces->blockSignals(true);
    ui_combo_queueInterfaces->setCurrentIndex(qiIndex);
    ui_combo_queueInterfaces->blockSignals(false);
    if (hasQueueInterfaceEditor(currentQueueInterface)) {
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

  Optimizer* currentOptimizer = getCurrentOptimizer();
  Q_ASSERT_X(currentOptimizer == 0 || m_optimizers.contains(
                 currentOptimizer->getIDString().toLower()), Q_FUNC_INFO,
             "Current optimizer is unknown to AbstractOptTab.");
  Q_ASSERT(m_optimizers.size() == ui_combo_optimizers->count());

  if (currentOptimizer) {
    int optIndex = m_optimizers.indexOf(currentOptimizer->getIDString().toLower());
    ui_combo_optimizers->blockSignals(true);
    ui_combo_optimizers->setCurrentIndex(optIndex);
    ui_combo_optimizers->blockSignals(false);
    ui_push_optimizerConfig->setEnabled(
      hasOptimizerEditor(currentOptimizer, getCurrentQueueInterface()));
  } else {
    ui_push_optimizerConfig->setEnabled(false);
  }
}

void AbstractOptTab::updateGUI()
{
  populateOptStepList();

  updateGUIQueueInterface();
  updateGUIOptimizer();

  ui_edit_user1->setText(m_search->getUser1());
  ui_edit_user2->setText(m_search->getUser2());
  ui_edit_user3->setText(m_search->getUser3());
  ui_edit_user4->setText(m_search->getUser4());

  ui_edit_locpath->setText(m_search->getLocWorkDir());
  ui_edit_description->setText(m_search->getDescription());

  bool wasBlocked = ui_cb_logErrorDirs->blockSignals(true);
  ui_cb_logErrorDirs->setChecked(m_search->logErrorDirs());
  ui_cb_logErrorDirs->blockSignals(wasBlocked);

  wasBlocked = ui_cb_cancelJobAfterTime->blockSignals(true);
  ui_cb_cancelJobAfterTime->setChecked(m_search->cancelJobAfterTime());
  ui_cb_cancelJobAfterTime->blockSignals(wasBlocked);

  wasBlocked = ui_spin_hoursForCancelJob->blockSignals(true);
  ui_spin_hoursForCancelJob->setValue(m_search->hoursForCancelJobAfterTime());
  ui_spin_hoursForCancelJob->setEnabled(m_search->cancelJobAfterTime());
  ui_spin_hoursForCancelJob->blockSignals(wasBlocked);
}

void AbstractOptTab::lockGUI()
{
  m_configDialogsReadOnly = true;
  ui_combo_optimizers->setDisabled(true);
  ui_combo_queueInterfaces->setDisabled(true);
  ui_push_add->setDisabled(true);
  ui_push_remove->setDisabled(true);
  ui_push_loadScheme->setDisabled(true);
  ui_push_optimizerConfig->setEnabled(
    hasOptimizerEditor(getCurrentOptimizer(), getCurrentQueueInterface()));
  ui_push_queueInterfaceConfig->setEnabled(getCurrentQueueInterface() &&
    hasQueueInterfaceEditor(getCurrentQueueInterface()));
  // Lock settings during a run.
  ui_edit_locpath->setReadOnly(true);
  ui_action_browse_locpath->setDisabled(true);
  ui_edit_description->setReadOnly(true);
  ui_cb_logErrorDirs->setDisabled(true);
  ui_edit_opt->setReadOnly(true);

  if (m_search->isReadOnly()) {
    ui_edit_user1->setReadOnly(true);
    ui_edit_user2->setReadOnly(true);
    ui_edit_user3->setReadOnly(true);
    ui_edit_user4->setReadOnly(true);
    ui_cb_cancelJobAfterTime->setDisabled(true);
    ui_spin_hoursForCancelJob->setDisabled(true);
  }
}

void AbstractOptTab::showHelp()
{
  if (!m_helpDialog) {
    m_helpDialog = new QWidget(m_dialog, Qt::Tool);
    m_helpDialog->setAttribute(Qt::WA_DeleteOnClose, true);
    m_helpDialog->setAttribute(Qt::WA_QuitOnClose, false);
    m_helpDialog->setWindowTitle(tr("Template Keywords"));

    QVBoxLayout* layout = new QVBoxLayout(m_helpDialog);

    QPlainTextEdit* textEdit = new QPlainTextEdit(m_helpDialog);
    textEdit->setReadOnly(true);
    textEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
    textEdit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    layout->addWidget(textEdit);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Close, m_helpDialog);
    connect(buttons, &QDialogButtonBox::rejected, m_helpDialog.data(), &QWidget::close);
    layout->addWidget(buttons);

    connect(m_helpDialog.data(), &QObject::destroyed, this, [this]() {
      m_helpDialog = nullptr;
    });
    m_helpDialog->resize(QSize(700, 600));
  }

  QPlainTextEdit* textEdit = m_helpDialog->findChild<QPlainTextEdit*>();
  if (textEdit)
    textEdit->setPlainText(m_search->getTemplateKeywordHelp());

  m_helpDialog->show();
  m_helpDialog->raise();
  m_helpDialog->activateWindow();
}

void AbstractOptTab::updateQueueInterface()
{
  if (m_configDialogsReadOnly)
    return;

  if (getCurrentOptStep() < 0)
    return;

  QueueInterface* currentQueueInterface = getCurrentQueueInterface();
  Q_ASSERT_X(currentQueueInterface == 0 || m_queueInterfaces.contains(
                 currentQueueInterface->getIDString().toLower()), Q_FUNC_INFO,
             "Current queue interface is unknown to AbstractOptTab.");

  unsigned int newQiIndex = ui_combo_queueInterfaces->currentIndex();

  Q_ASSERT(static_cast<int>(newQiIndex) < m_queueInterfaces.size());

  QueueInterface* qi = m_search->queueInterface(getCurrentOptStep());

  if (hasQueueInterfaceEditor(qi)) {
    ui_push_queueInterfaceConfig->setEnabled(true);
  } else {
    ui_push_queueInterfaceConfig->setEnabled(false);
  }

  emit queueInterfaceChanged(getCurrentOptStep(),
                             m_queueInterfaces[newQiIndex].toStdString());
}

void AbstractOptTab::updateOptimizer()
{
  if (m_configDialogsReadOnly)
    return;

  if (getCurrentOptStep() < 0)
    return;

  Optimizer* currentOptimizer = getCurrentOptimizer();
  Q_ASSERT_X(currentOptimizer == 0 || m_optimizers.contains(
                 currentOptimizer->getIDString().toLower()), Q_FUNC_INFO,
             "Current optimizer is unknown to AbstractOptTab.");

  unsigned int newOptimizerIndex = ui_combo_optimizers->currentIndex();

  Q_ASSERT(static_cast<int>(newOptimizerIndex) < m_optimizers.size());

  Optimizer* o = m_search->optimizer(getCurrentOptStep());

  if (hasOptimizerEditor(o, getCurrentQueueInterface())) {
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
  Q_ASSERT(hasQueueInterfaceEditor(getCurrentQueueInterface()));

  QDialog* d = createQueueInterfaceEditor(m_dialog, m_search, getCurrentQueueInterface());
  Q_ASSERT(d != 0);

  if (m_configDialogsReadOnly)
    makeConfigurationDialogReadOnly(d, !m_search->isReadOnly());

  d->show();
  d->exec();
  delete d;
}

void AbstractOptTab::configureOptimizer()
{
  Q_ASSERT(getCurrentOptimizer());
  if (!hasOptimizerEditor(getCurrentOptimizer(), getCurrentQueueInterface()))
    return;

  QDialog* d = createOptimizerEditor(m_dialog, m_search, getCurrentOptimizer(),
                                     getCurrentQueueInterface());
  Q_ASSERT(d != 0);

  if (m_configDialogsReadOnly)
    makeConfigurationDialogReadOnly(d, !m_search->isReadOnly());

  d->show();
  d->exec();
  delete d;
}

QStringList AbstractOptTab::getTemplateNames(size_t /*optStep*/)
{
  if (!m_isInitialized) {
    return QStringList();
  }
  // Check the current optimization step (gaurd against a partial import).
  Optimizer* optimizer = getCurrentOptimizer();
  QueueInterface* queue = getCurrentQueueInterface();
  if (!optimizer || !queue)
    return QStringList();

  QStringList templateNames = optimizer->getOptimizerTemplateFileNames();
  templateNames.append(queue->getQueueInterfaceTemplateFileNames());
  for (const auto& assetName : optimizer->getOptimizerInputAssetNames()) {
    if (!templateNames.contains(assetName))
      templateNames.append(assetName);
  }
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
  for (int i = 0; i < ui_combo_templates->count(); ++i) {
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

  if (static_cast<int>(m_search->getNumOptSteps()) != ui_list_optStep->count()) {
    populateOptStepList();
  }

  Q_ASSERT(optStepIndex >= 0 && optStepIndex < static_cast<int>(m_search->getNumOptSteps()));

  // Update text edit widget
  Q_ASSERT(getTemplateNames(optStepIndex).contains(templateName));
  Optimizer* currentOptimizer = getCurrentOptimizer();
  QueueInterface* currentQueueInterface = getCurrentQueueInterface();
  const bool optimizerInputAsset =
    isOptimizerInputAssetName(currentOptimizer, currentQueueInterface, templateName);
  const bool queueTemplate = currentQueueInterface &&
    currentQueueInterface->getQueueInterfaceTemplateFileNames().contains(templateName);
  const bool optimizerTemplate = currentOptimizer &&
    currentOptimizer->getOptimizerTemplateFileNames().contains(templateName);

  QString text;
  if (optimizerInputAsset) {
    // Input asset files are stored as a map; show on "<id> <file>" per line.
    const OptimizerInputAssetMap assets =
      m_search->getOptimizerInputAssets(optStepIndex, templateName.toStdString());
    QStringList lines;
    for (const auto& asset : assets) {
      lines.append(QString::fromStdString(asset.first) + " " +
                   QString::fromStdString(asset.second));
    }
    text = lines.join("\n");
    ui_edit_opt->setToolTip(tr("Input asset: one entry per line as '<element or system> <file>'. "
         "The file is a path or a %fileContents:/path% entry; assets are "
         "references only, with no keyword interpretation. File paths cannot "
         "contain ';' or '#'."));
  } else if (queueTemplate && !optimizerTemplate) {
    text = m_search->getQueueInterfaceTemplate(optStepIndex, templateName.toStdString()).c_str();
    ui_edit_opt->setToolTip(tr("Template: paste the file content, or use %fileContents:/path% / "
         "%copyFile:/path% to insert a file. Keywords are interpreted at run time."));
  } else if (optimizerTemplate && !queueTemplate) {
    text = m_search->getOptimizerTemplate(optStepIndex, templateName.toStdString()).c_str();
    ui_edit_opt->setToolTip(tr("Template: paste the file content, or use %fileContents:/path% / "
         "%copyFile:/path% to insert a file. Keywords are interpreted at run time."));
  } else {
    Common::error(QString("Template name %1 is ambiguous or unknown.")
                    .arg(templateName));
  }

  ui_edit_opt->blockSignals(true);
  ui_edit_opt->setPlainText(text);
  ui_edit_opt->blockSignals(false);
}

void AbstractOptTab::saveCurrentTemplate()
{
  if (m_configDialogsReadOnly || m_search->isReadOnly())
    return;

  int optStepIndex = getCurrentOptStep();
  QStringList filenames = getTemplateNames(optStepIndex);
  int templateInd = ui_combo_templates->currentIndex();
  QString templateName = ui_combo_templates->currentText();
  Q_ASSERT(templateInd >= 0 && templateInd < filenames.size());
  Q_ASSERT(templateName.compare(filenames.at(templateInd)) == 0);

  if (static_cast<int>(m_search->getNumOptSteps()) != ui_list_optStep->count())
    populateOptStepList();

  Q_ASSERT(optStepIndex >= 0 && optStepIndex < static_cast<int>(m_search->getNumOptSteps()));

  // Update templates from the text edit widget.
  QString text = ui_edit_opt->document()->toPlainText();
  Optimizer* currentOptimizer = getCurrentOptimizer();
  QueueInterface* currentQueueInterface = getCurrentQueueInterface();
  const bool optimizerInputAsset =
    isOptimizerInputAssetName(currentOptimizer, currentQueueInterface, templateName);
  const bool queueTemplate = currentQueueInterface &&
    currentQueueInterface->getQueueInterfaceTemplateFileNames().contains(templateName);
  const bool optimizerTemplate = currentOptimizer &&
    currentOptimizer->getOptimizerTemplateFileNames().contains(templateName);

  if (optimizerInputAsset) {
    // Convert asset inputs from "<id> <file>" to a map.
    OptimizerInputAssetMap assetFiles;
    const QStringList lines = text.split('\n');
    for (const auto& line : lines) {
      if (line.trimmed().isEmpty())
        continue;
      QString id, fileEntry;
      // A line that cannot be read yet (the user is still typing it) leaves
      //   the saved entries as they were, like the other settings do.
      if (!parseInputAssetLine(line, id, fileEntry))
        return;
      // Keep an absolute path, so the entry does not depend on where the
      //   program is started the next time.
      assetFiles[id.toStdString()] = QFileInfo(fileEntry).absoluteFilePath().toStdString();
    }
    m_search->setOptimizerInputAssets(optStepIndex, templateName.toStdString(), assetFiles);
  } else if (queueTemplate && !optimizerTemplate) {
    m_search->setQueueInterfaceTemplate(
      optStepIndex, templateName.toStdString(), text.toStdString());
  } else if (optimizerTemplate && !queueTemplate) {
    m_search->setOptimizerTemplate(optStepIndex, templateName.toStdString(), text.toStdString());
  } else {
    Common::error(QString("Template name %1 is ambiguous or unknown.")
                    .arg(templateName));
  }
}

void AbstractOptTab::updateUserValues()
{
  if (m_search->isReadOnly())
    return;

  m_search->setUser1(ui_edit_user1->text());
  m_search->setUser2(ui_edit_user2->text());
  m_search->setUser3(ui_edit_user3->text());
  m_search->setUser4(ui_edit_user4->text());
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
  if (m_configDialogsReadOnly)
    return;

  // Reimplement in derived class if Optimizer generic data is needed
  const int maxSteps = m_search->getNumOptSteps();
  const int currentOptStep = getCurrentOptStep();
  Q_ASSERT(currentOptStep >= 0 && currentOptStep < maxSteps);

  m_search->appendOptStep();

  populateOptStepList();
}

void AbstractOptTab::removeCurrentOptStep()
{
  if (m_configDialogsReadOnly)
    return;

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

void AbstractOptTab::browseLocWorkDir()
{
  QString startDir = ui_edit_locpath->text().trimmed();
  if (startDir.isEmpty() || !QDir::isAbsolutePath(startDir))
    startDir = QDir::homePath();
  const QString dir = QFileDialog::getExistingDirectory(
    m_dialog, tr("Select local working directory"), startDir,
    QFileDialog::ShowDirsOnly | QFileDialog::DontUseNativeDialog);
  if (!dir.isEmpty()) {
    ui_edit_locpath->setText(dir);
    m_search->setLocWorkDir(dir);
  }
}

void AbstractOptTab::saveScheme()
{
  QString filename = QFileDialog::getSaveFileName(
    nullptr, tr("Save Optimization Scheme as..."), QDir::homePath(),
    "*.*", nullptr, QFileDialog::DontUseNativeDialog);

  // User canceled
  if (filename.isEmpty())
    return;

  writeSchemeFile(filename);
}

void AbstractOptTab::loadScheme()
{
  if (m_configDialogsReadOnly)
    return;

  QString filename = QFileDialog::getOpenFileName(
    nullptr, tr("Select Optimization Scheme to load..."), QDir::homePath(),
    "*.*", 0, QFileDialog::DontUseNativeDialog);

  // User canceled
  if (filename.isEmpty())
    return;

  readSchemeFile(filename);
}
}
