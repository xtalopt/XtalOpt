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

#include <xtalopt/ui/dialog.h>

#include "ui_dialog.h"

#include <globalsearch/optimizer.h>
#include <globalsearch/tracker.h>
#include <xtalopt/testing/xtalopttest.h>

#include <xtalopt/ui/tab_struc.h>
#include <xtalopt/ui/tab_opt.h>
#include <xtalopt/ui/tab_search.h>
#include <xtalopt/ui/tab_mo.h>
#include <xtalopt/ui/tab_progress.h>
#include <xtalopt/ui/tab_plot.h>
#include <xtalopt/ui/tab_log.h>
#include <xtalopt/ui/tab_about.h>
#include <xtalopt/xtalopt.h>

#include <QSettings>
#include <QUrl>
#include <QtConcurrent>

#include <QDesktopServices>
#include <QFileDialog>
#include <QMessageBox>

using namespace GlobalSearch;
using namespace std;

namespace XtalOpt {

XtalOptDialog::XtalOptDialog(QWidget* parent, Qt::WindowFlags f,
                             bool interactive, XtalOpt* xtalopt)
  : AbstractDialog(parent, f)
{
  setWindowFlags(Qt::Window);
  ui = new Ui::XtalOptDialog;
  ui->setupUi(this);

  connect(ui->push_import, SIGNAL(clicked()), this, SLOT(importSettings()));
  connect(ui->push_export, SIGNAL(clicked()), this, SLOT(exportSettings()));

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
  m_tab_search = new TabSearch(this, xtalopt);
  m_tab_mo = new TabMo(this, xtalopt);
  m_tab_progress = new TabProgress(this, xtalopt);
  m_tab_plot = new TabPlot(this, xtalopt);
  m_tab_log = new TabLog(this, xtalopt);
  m_tab_about = new TabAbout(this, xtalopt);

  // Populate tab widget
  ui->tabs->clear();
  ui->tabs->addTab(m_tab_struc->getTabWidget(), tr("&Structure Limits"));
  ui->tabs->addTab(m_tab_opt->getTabWidget(), tr("&Optimization Settings"));
  ui->tabs->addTab(m_tab_search->getTabWidget(), tr("&Search Settings"));
  ui->tabs->addTab(m_tab_mo->getTabWidget(), tr("&Multiobjective Search"));
  ui->tabs->addTab(m_tab_progress->getTabWidget(), tr("&Progress"));
  ui->tabs->addTab(m_tab_plot->getTabWidget(), tr("&Plot"));
  ui->tabs->addTab(m_tab_log->getTabWidget(), tr("&Log"));
  ui->tabs->addTab(m_tab_about->getTabWidget(), tr("&About"));

  initialize();

  QSettings settings;
  if (interactive &&
      settings.value("xtalopt/showTutorialLink", true).toBool()) {
    showTutorialDialog();
  }
}

XtalOptDialog::~XtalOptDialog()
{
  this->hide();

  writeSettings();
}

void XtalOptDialog::setMolecule(GlobalSearch::Molecule* molecule)
{
  if (m_molecule == molecule || !molecule) {
    return;
  }
  m_molecule = molecule;
}

void XtalOptDialog::closeEvent(QCloseEvent *e)
{
  // Show "quit" dialog before closing main window
  QMessageBox::StandardButton reply;
  reply = QMessageBox::question(this, "Quit", "Quit XtalOpt now?",
                                QMessageBox::Yes|QMessageBox::No);
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

void XtalOptDialog::beginPlotOnlyMode()
{
  m_tab_progress->blockSignals(true);

  // Make sure xtalopt is in readOnly mode
  m_search->readOnly = true;

  // A QWidget will not display by itself if its parent is not displayed.
  // Thus, we need to set the parent to nullptr.
  m_tab_plot->getTabWidget()->setParent(nullptr);

  m_tab_plot->getTabWidget()->setWindowTitle("XtalOpt Plot");
  m_tab_plot->getTabWidget()->show();
}

void XtalOptDialog::saveSession()
{
  // Notify if this was user requested.
  bool notify = false;
  if (sender() == ui->push_save)
    notify = true;

  QtConcurrent::run([this, notify]() { this->m_search->save("", notify); });
}

void XtalOptDialog::startSearch()
{
  if (m_search->testingMode) {
    m_test = new XtalOptTest(qobject_cast<XtalOpt*>(m_search));
    QtConcurrent::run(m_test, &XtalOptTest::start);
  } else {
    QtConcurrent::run(qobject_cast<XtalOpt*>(m_search), &XtalOpt::startSearch);
  }
}

bool XtalOptDialog::importSettings()
{
  // EXPERIMENTAL FEATURE: Read a CLI input file and initialize
  //   the GUI entries. Option should be used carefully; some
  //   entries of GUI might be set improperly, especially
  //   Optimizer/Queue settings.

  // Launch file dialog
  QString newFilename = QFileDialog::getOpenFileName(
    this, QString("Select xtalopt input settings file"),
    QDir::homePath(), "All files (*.*)",
    0, QFileDialog::DontUseNativeDialog);

  // If a valid file is selected
  if (!newFilename.isEmpty()) {
    XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

    bool read = XtalOpt::importSettings_(newFilename, *xtalopt);
    m_tab_struc->updateGUI();
    m_tab_struc->updateCompositionTable();
    m_tab_opt->updateGUI();
    m_tab_search->updateGUI();
    m_tab_search->showSeeds();
    m_tab_mo->updateGUI();
    // This is important: soft/hard exit should be ignored in GUI
    xtalopt->m_softExit = false;
    xtalopt->m_hardExit = false;
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
  // EXPERIMENTAL FEATURE: Output GUI settings to a file
  //   for CLI input. Optimizer/Queue settings are not fully exported!

  // Launch file dialog
  QString newFilename = QFileDialog::getSaveFileName(
    this, QString("Select a file to write settings file"),
      QDir::homePath(), "All files (*.*)",
      0, QFileDialog::DontUseNativeDialog);

  // If a valid file is selected
  if (!newFilename.isEmpty()) {
    XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);
    bool write = XtalOpt::exportSettings_(newFilename, xtalopt);
    if (!write) {
      QString errmsg = QString("Failed to write settings from the file:\n'%1'").arg(newFilename);
      errorPromptWindow(errmsg);
      return false;
    }
  }

  return true;
}

void XtalOptDialog::showTutorialDialog() const
{
  QMessageBox mbox;
  mbox.setText("There is a user manual available for new XtalOpt users at\n\n"
               "https://xtalopt.github.io/xtalopt.html"
               "\n\nWould you like to go there now?");
  mbox.setIcon(QMessageBox::Information);
  QPushButton* yes = mbox.addButton(tr("&Yes"), QMessageBox::YesRole);
  QPushButton* no = mbox.addButton(tr("&No"), QMessageBox::NoRole);
  QPushButton* never =
    mbox.addButton(tr("No, stop asking!"), QMessageBox::NoRole);

  mbox.exec();

  QAbstractButton* clicked = mbox.clickedButton();

  if (clicked == yes) {
    QDesktopServices::openUrl(
      QUrl("https://xtalopt.github.io/xtalopt.html", QUrl::TolerantMode));
  } else if (clicked == never) {
    QSettings settings;
    settings.setValue("xtalopt/showTutorialLink", false);
  }
  // else ("no" clicked) just return;

  return;
}

void XtalOptDialog::errorPromptWindow(const QString& instr)
{
  QMessageBox msgBox;
  msgBox.setText(instr);
  msgBox.exec();
}
}
