/**********************************************************************
  MolecularDialog - the dialog for molecular XtalOpt

  Copyright (C) 2017 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include "moleculardialog.h"

#include "ui_moleculardialog.h"

#include <globalsearch/optimizer.h>
#include <globalsearch/tracker.h>
#include <xtalopt/testing/xtalopttest.h>

#include "tab_molecularinit.h"
#include "tab_molecularopt.h"
#include <xtalopt/ui/tab_edit.h>
#include <xtalopt/ui/tab_log.h>
#include <xtalopt/ui/tab_plot.h>
#include <xtalopt/ui/tab_progress.h>
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

MolecularXtalOptDialog::MolecularXtalOptDialog(QWidget* parent,
                                               Qt::WindowFlags f,
                                               bool interactive,
                                               XtalOpt* xtalopt)
  : AbstractDialog(parent, f)
{
  setWindowFlags(Qt::Window);
  ui = new Ui::MolecularXtalOptDialog;
  ui->setupUi(this);
  ui_push_begin = ui->push_begin;
  ui_push_save = ui->push_save;
  ui_push_resume = ui->push_resume;
  ui_label_opt = ui->label_opt;
  ui_label_run = ui->label_run;
  ui_label_fail = ui->label_fail;
  ui_label_prog = ui->label_prog;
  ui_progbar = ui->progbar;
  ui_tabs = ui->tabs;

  // Initialize vars, connections, etc
  if (!xtalopt) {
    xtalopt = new XtalOpt(this);
    m_ownsOptBase = true;
  }

  m_opt = xtalopt;

  // Initialize tabs
  m_tab_init = new TabMolecularInit(this, xtalopt);
  m_tab_edit = new TabEdit(this, xtalopt);
  m_tab_opt = new TabMolecularOpt(this, xtalopt);
  m_tab_progress = new TabProgress(this, xtalopt);
  m_tab_plot = new TabPlot(this, xtalopt);
  m_tab_log = new TabLog(this, xtalopt);

  // Populate tab widget
  ui->tabs->clear();
  ui->tabs->addTab(m_tab_init->getTabWidget(), tr("&Structure Limits"));
  ui->tabs->addTab(m_tab_edit->getTabWidget(), tr("Optimization &Settings"));
  ui->tabs->addTab(m_tab_opt->getTabWidget(), tr("&Search Settings"));
  ui->tabs->addTab(m_tab_progress->getTabWidget(), tr("&Progress"));
  ui->tabs->addTab(m_tab_plot->getTabWidget(), tr("&Plot"));
  ui->tabs->addTab(m_tab_log->getTabWidget(), tr("&Log"));

  initialize();

  QSettings settings;
  if (interactive &&
      settings.value("xtalopt/showTutorialLink", true).toBool()) {
    showTutorialDialog();
  }
}

MolecularXtalOptDialog::~MolecularXtalOptDialog()
{
  this->hide();

  writeSettings();
}

void MolecularXtalOptDialog::setMolecule(GlobalSearch::Molecule* molecule)
{
  if (m_molecule == molecule || !molecule) {
    return;
  }
  m_molecule = molecule;
}

void MolecularXtalOptDialog::beginPlotOnlyMode()
{
  // Make sure xtalopt is in readOnly mode
  m_opt->readOnly = true;

  // A QWidget will not display by itself if its parent is not displayed.
  // Thus, we need to set the parent to nullptr.
  m_tab_plot->getTabWidget()->setParent(nullptr);

  m_tab_plot->getTabWidget()->setWindowTitle("XtalOpt Plot");
  m_tab_plot->getTabWidget()->show();
}

void MolecularXtalOptDialog::saveSession()
{
  // Notify if this was user requested.
  bool notify = false;
  if (sender() == ui->push_save)
    notify = true;
  QtConcurrent::run(m_opt, &OptBase::save, QString(""), notify);
}

void MolecularXtalOptDialog::startSearch()
{
  if (m_opt->testingMode) {
    m_test = new XtalOptTest(qobject_cast<XtalOpt*>(m_opt));
    QtConcurrent::run(m_test, &XtalOptTest::start);
  } else {
    QtConcurrent::run(qobject_cast<XtalOpt*>(m_opt), &XtalOpt::startSearch);
  }
}

void MolecularXtalOptDialog::showTutorialDialog() const
{
  QMessageBox mbox;
  mbox.setText("There is a tutorial available for new XtalOpt users at\n\n"
               "http://xtalopt.github.io/tut.html"
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
      QUrl("http://xtalopt.github.io/tut.html", QUrl::TolerantMode));
  } else if (clicked == never) {
    QSettings settings;
    settings.setValue("xtalopt/showTutorialLink", false);
  }
  // else ("no" clicked) just return;

  return;
}
}
