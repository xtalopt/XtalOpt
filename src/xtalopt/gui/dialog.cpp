/**********************************************************************
  XtalOptDialog - The main GUI window: creates the tabs and connects them to the search engine.

  Copyright (C) 2009-2011 by David Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <xtalopt/gui/dialog.h>

#include "ui_dialog.h"

#include <search/structure.h>
#include <search/optimizer.h>
#include <search/tracker.h>
#include <common/compatibility/platform_defs.h>
#include <common/fileutils.h>
#include <atoms/gui/structureviewdialog.h>
#include <generatexrd/gui/xrdviewdialog.h>

#include <xtalopt/gui/tab_struc.h>
#include <xtalopt/gui/tab_opt.h>
#include <xtalopt/gui/tab_search.h>
#include <xtalopt/gui/tab_mo.h>
#include <xtalopt/gui/tab_progress.h>
#include <xtalopt/gui/tab_plot.h>
#include <xtalopt/gui/tab_log.h>
#include <xtalopt/gui/tab_about.h>
#include <xtalopt/xtalopt.h>
#include <xtalopt/settings.h>

#include <QPushButton>
#include <QReadLocker>
#include <QtConcurrent>

#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QPointer>

using namespace Search;
using namespace std;

namespace XtalOpt {

namespace {

QString defaultSaveDir()
{
#if GS_WINDOWS
  return QStringLiteral("C:/");
#else
  return QDir::homePath();
#endif
}

void resetImportedLocalWorkDirIfNeeded(XtalOpt* xtalopt)
{
  if (!xtalopt)
    return;

  const QString path = xtalopt->getLocWorkDir().trimmed();
  if (path.isEmpty())
    return;

  const QString cleanPath = QDir::cleanPath(path);
  const QString homePath = QDir::cleanPath(QDir::homePath());

  // Do not show "a default working directory" other than generic value;
  //   avoid root or HOME!
  if (QDir(cleanPath).isRoot() || cleanPath == homePath) {
    xtalopt->setLocWorkDir(Settings::defaultValue("localWorkingDirectory"));
  }
}

} // namespace

XtalOptDialog::XtalOptDialog(QWidget* parent, Qt::WindowFlags f, XtalOpt* xtalopt)
  : AbstractDialog(parent, f)
{
  setWindowFlags(Qt::Window);
  ui = new Ui::XtalOptDialog;
  ui->setupUi(this);

  connect(ui->push_import, &QPushButton::clicked, this, &XtalOptDialog::importSettings);
  connect(ui->push_export, &QPushButton::clicked, this, &XtalOptDialog::exportSettings);

  ui_push_begin = ui->push_begin;
  ui_push_save = ui->push_save;
  ui_push_resume = ui->push_resume;
  ui_label_opt = ui->label_opt;
  ui_label_run = ui->label_run;
  ui_label_tot = ui->label_tot;
  ui_label_fail = ui->label_fail;
  ui_label_prog = ui->label_prog;
  ui_progbar = ui->progbar;
  ui_push_import = ui->push_import;
  ui_push_export = ui->push_export;
  ui_tabs = ui->tabs;

  ui_push_import->setAutoDefault(false);

  if (!xtalopt) {
    xtalopt = new XtalOpt(this);
    m_ownsSearchBase = true;
  }

  m_search = xtalopt;

  // Initialize tabs
  m_tab_struc = new TabStruc(this, xtalopt);
  m_tab_opt = new TabOpt(this, xtalopt);
  m_tab_mo = new TabMo(this, xtalopt);
  m_tab_search = new TabSearch(this, xtalopt);
  m_tab_progress = new TabProgress(this, xtalopt);
  m_tab_plot = new TabPlot(this, xtalopt);
  m_tab_log = new TabLog(this, xtalopt);
  m_tab_about = new TabAbout(this, xtalopt);

  // Populate tab widget
  ui->tabs->clear();
  ui->tabs->addTab(m_tab_struc->getTabWidget(), tr("&Structure"));
  ui->tabs->addTab(m_tab_opt->getTabWidget(), tr("&Optimization"));
  ui->tabs->addTab(m_tab_mo->getTabWidget(), tr("Objectives and Constraints"));
  ui->tabs->addTab(m_tab_search->getTabWidget(), tr("&Search"));
  ui->tabs->addTab(m_tab_progress->getTabWidget(), tr("&Progress"));
  ui->tabs->addTab(m_tab_plot->getTabWidget(), tr("&Plot"));
  ui->tabs->addTab(m_tab_log->getTabWidget(), tr("&Log"));
  ui->tabs->addTab(m_tab_about->getTabWidget(), tr("&About"));

  // Do not use Enter for a main window button.
  for (auto* button : findChildren<QPushButton*>()) {
    button->setDefault(false);
    button->setAutoDefault(false);
  }

  initialize();
  ui->tabs->setFocus();
  ui_push_save->setEnabled(true);
}

XtalOptDialog::~XtalOptDialog()
{
  this->hide();
  delete m_tab_about;
  delete m_tab_log;
  delete m_tab_plot;
  delete m_tab_progress;
  delete m_tab_search;
  delete m_tab_mo;
  delete m_tab_opt;
  delete m_tab_struc;
  delete ui;
}

void XtalOptDialog::closeEvent(QCloseEvent *e)
{
  // Show "quit" dialog before closing main window
  QMessageBox::StandardButton reply =
    QMessageBox::question(this, tr("Quit"), tr("Quit XtalOpt now?"),
                          QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
  if (reply == QMessageBox::Yes)
    e->accept();
  else
    e->ignore();
}

void XtalOptDialog::keyPressEvent(QKeyEvent *e)
{
  // Prevent closing with "Esc" key: applies only to the "main window"
  if (e->key() == Qt::Key_Escape)
    return;
  QDialog::keyPressEvent(e);
}

void XtalOptDialog::lockGUI()
{
  AbstractDialog::lockGUI();
  ui_push_begin->setDisabled(true);
  ui_push_resume->setDisabled(true);
  ui_push_import->setDisabled(true);
  ui_push_save->setEnabled(true);
}

void XtalOptDialog::beginPlotOnlyMode()
{
  m_tab_progress->disconnectGUI();

  // A QWidget will not display by itself if its parent is not displayed.
  // Thus, we need to set the parent to nullptr.
  m_tab_plot->getTabWidget()->setParent(nullptr);

  m_tab_plot->getTabWidget()->setWindowTitle("XtalOpt (Plot)");
  m_tab_plot->getTabWidget()->show();
  m_tab_plot->refreshPlot();
}

QString XtalOptDialog::sessionStateFilePath() const
{
  auto* xtalopt = qobject_cast<XtalOpt*>(m_search);
  if (!xtalopt || xtalopt->getLocWorkDir().isEmpty())
    return QString();

  return xtalopt->stateFilePath();
}

bool XtalOptDialog::prepareResumeSession(const QString& filename)
{
  Q_UNUSED(filename);
  auto* xo = qobject_cast<XtalOpt*>(m_search);
  if (!xo)
    return false;

  // XtalOpt chooses active vs read-only resume before state is restored.
  QMessageBox::StandardButton reply = QMessageBox::question(this, "Load Session",
                          "Load this search and resume with submitting jobs?\n\n"
                          "Choose No to load the session read-only.",
                          QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                          QMessageBox::Yes);
  if (reply == QMessageBox::Cancel)
    return false;

  xo->setRunMode(reply == QMessageBox::No ? XtalOpt::RunModeReadOnly : XtalOpt::RunModeGui);
  return true;
}

void XtalOptDialog::saveSession()
{
  // Notify if this was user requested.
  bool notify = false;
  if (sender() == ui->push_save)
    notify = true;

  if (m_search->isSessionStarting()) {
    errorPromptWindow("Cannot save while the search is starting.");
    return;
  }

  bool hasStructures = false;
  {
    QReadLocker locker(m_search->tracker()->rwLock());
    hasStructures = !m_search->tracker()->list()->isEmpty();
  }

  if (m_search->isReadOnly() || !hasStructures) {
    XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

    QString filename = QFileDialog::getSaveFileName(
      this, QString("Save search state file"),
      QDir(defaultSaveDir()).filePath("xtalopt.state"), "XtalOpt state (*.state);;All files (*.*)",
      0, QFileDialog::DontUseNativeDialog);

    if (filename.isEmpty())
      return;

    if (!filename.endsWith(".state", Qt::CaseInsensitive))
      filename.append(".state");

    if (!xtalopt || !xtalopt->saveStateFile(filename)) {
      errorPromptWindow(QString("Failed to save state file to:\n'%1'")
                          .arg(filename));
    }
    return;
  }

  const QString filename = sessionStateFilePath();
  QPointer<XtalOptDialog> self(this);
  (void)QtConcurrent::run([self, notify, filename]() {
    if (!self || filename.isEmpty())
      return;

    XtalOpt* xtalopt = qobject_cast<XtalOpt*>(self->m_search);
    if (xtalopt)
      xtalopt->saveSessionState(filename, notify);
  });
}

void XtalOptDialog::showStructureViewer(Search::Structure* structure)
{
  if (!structure)
    return;

  auto* viewer = new Atoms::StructureViewDialog(this);
  viewer->setAttribute(Qt::WA_DeleteOnClose, true);

  // Copy the geometry for the view under a short lock only.
  Atoms::Geometry geometry;
  QString tag;
  {
    QReadLocker locker(&structure->lock());
    geometry = *structure;
    tag = structure->getTag();
  }
  viewer->displayStructure(geometry, tag);
}

void XtalOptDialog::showXrdViewer(Search::Structure* structure)
{
  if (!structure)
    return;

  auto* viewer = new GenerateXrd::XrdViewDialog(this);
  viewer->setAttribute(Qt::WA_DeleteOnClose, true);

  // Copy the geometry for the xrd pattern under a short lock only.
  Atoms::Geometry geometry;
  QString tag;
  {
    QReadLocker locker(&structure->lock());
    geometry = *structure;
    tag = structure->getTag();
  }
  viewer->displayStructure(geometry, tag);
}

void XtalOptDialog::startSearch()
{
  auto* xo = qobject_cast<XtalOpt*>(m_search);
  QPointer<XtalOptDialog> self(this);
  (void)QtConcurrent::run([self, xo]() {
    if (self)
      xo->startSearch();
  });
}

void XtalOptDialog::resumeSession_(const QString& filename)
{
  // Check that the dialog is still open.
  QPointer<XtalOptDialog> self(this);

  startProgressUpdate(tr("Resuming session..."), 0, 0);

  // resumeSearch() itself resets the engine session state on failure.
  auto* xo = qobject_cast<XtalOpt*>(m_search);
  bool settingsOnlyLoaded = false;
  if (xo && !xo->resumeSearch(filename, &settingsOnlyLoaded) && settingsOnlyLoaded) {
    xo->setRunMode(XtalOpt::RunModeGui);
    resetImportedLocalWorkDirIfNeeded(xo);
    if (self) {
      QMetaObject::invokeMethod(self.data(), "updateGUI", Qt::QueuedConnection);
      QMetaObject::invokeMethod(
        self.data(), "errorPromptWindow", Qt::QueuedConnection,
        Q_ARG(QString,
          tr("No structures were found for this state file.\n\n"
             "The settings were loaded into the GUI, but no search was resumed. "
             "Review the settings carefully and press Begin to start a new run.")));
    }
  }

  if (self)
    self->stopProgressUpdate();
}

bool XtalOptDialog::importSettings()
{
  // Import an input file (best effort!). The search settings
  //   are applied directly, but optimizer and queue details should
  //   still be check by the user!

  // Launch file dialog
  QString newFilename = QFileDialog::getOpenFileName(
    this, QString("Import settings from XtalOpt CLI input file"),
    defaultSaveDir(), "All files (*.*)", 0, QFileDialog::DontUseNativeDialog);

  // If a valid file is selected
  if (!newFilename.isEmpty()) {
    XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

    bool read =
      xtalopt->loadInputFile(newFilename, /*bestEffort*/ true);
    resetImportedLocalWorkDirIfNeeded(xtalopt);
    updateGUI();
    // If reading is not fully ok; we will still update the GUI, but
    //   will let the user know that something was off!
    if (!read) {
      QString errmsg = QString("Failed to read *some* settings from the file:\n'%1'").arg(newFilename);
      errorPromptWindow(errmsg);
    }
  }

  return true;
}

bool XtalOptDialog::exportSettings()
{
  // Export an input file (best effort!). The template and optimizer/queue details
  //   are especially written as placeholders.

  // Launch file dialog
  QString newFilename = QFileDialog::getSaveFileName(
    this, QString("Export settings to XtalOpt CLI input file"), defaultSaveDir(), "All files (*.*)",
      0, QFileDialog::DontUseNativeDialog);

  // If a valid file is selected
  if (!newFilename.isEmpty()) {
    XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);
    bool write = xtalopt->saveInputFile(newFilename);
    if (!write) {
      QString errmsg = QString("Failed to write settings to the file:\n'%1'").arg(newFilename);
      errorPromptWindow(errmsg);
      return false;
    }
  }

  return true;
}

void XtalOptDialog::errorPromptWindow(const QString& instr)
{
  QMessageBox msgBox;
  msgBox.setText(instr);
  msgBox.exec();
}
}
