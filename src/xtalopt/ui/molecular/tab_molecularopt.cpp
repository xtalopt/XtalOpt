/**********************************************************************
  TabMolecularOpt - the tab for molecular optimization options in XtalOpt.

  Copyright (C) 2017 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include "tab_molecularopt.h"

#include <xtalopt/xtalopt.h>
#include <xtalopt/ui/dialog.h>

#include <QDebug>
#include <QSettings>

#include <QFileDialog>
#include <QMessageBox>

using namespace std;

namespace XtalOpt {

  TabMolecularOpt::TabMolecularOpt(GlobalSearch::AbstractDialog *parent,
                                   XtalOpt *p ) :
    AbstractTab(parent, p)
  {
    ui.setupUi(m_tab_widget);

    // Optimization connections
    // Initial generation
    connect(ui.spin_numInitial, SIGNAL(valueChanged(int)),
            this, SLOT(updateOptimizationInfo()));

    // Seeds
    connect(ui.push_addSeed, SIGNAL(clicked()),
            this, SLOT(addSeed()));
    connect(ui.push_removeSeed, SIGNAL(clicked()),
            this, SLOT(removeSeed()));
    connect(ui.list_seeds, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
            this, SLOT(addSeed(QListWidgetItem*)));

    // Search params
    connect(ui.spin_popSize, SIGNAL(valueChanged(int)),
            this, SLOT(updateOptimizationInfo()));
    connect(ui.spin_contStructs, SIGNAL(valueChanged(int)),
            this, SLOT(updateOptimizationInfo()));
    connect(ui.cb_limitRunningJobs, SIGNAL(stateChanged(int)),
            this, SLOT(updateOptimizationInfo()));
    connect(ui.spin_runningJobLimit, SIGNAL(valueChanged(int)),
            this, SLOT(updateOptimizationInfo()));
    connect(ui.spin_failLimit, SIGNAL(valueChanged(int)),
            this, SLOT(updateOptimizationInfo()));
    connect(ui.combo_failAction, SIGNAL(currentIndexChanged(int)),
            this, SLOT(updateOptimizationInfo()));
    connect(ui.spin_cutoff, SIGNAL(valueChanged(int)),
            this, SLOT(updateOptimizationInfo()));


    initialize();
  }

  TabMolecularOpt::~TabMolecularOpt()
  {
  }

  void TabMolecularOpt::writeSettings(const QString &filename)
  {
    SETTINGS(filename);

    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    settings->beginGroup("xtalopt/opt/");

    // config version
    const int version = 1;
    settings->setValue("version",               version);

    // Initial generation
    settings->setValue("opt/numInitial",        xtalopt->numInitial);

    // Search parameters
    settings->setValue("opt/popSize",           xtalopt->popSize);
    settings->setValue("opt/contStructs",       xtalopt->contStructs);
    settings->setValue("opt/limitRunningJobs",  xtalopt->limitRunningJobs);
    settings->setValue("opt/runningJobLimit",   xtalopt->runningJobLimit);
    settings->setValue("opt/failLimit",         xtalopt->failLimit);
    settings->setValue("opt/failAction",        xtalopt->failAction);
    settings->setValue("opt/cutoff",            xtalopt->cutoff);

    settings->endGroup();

    DESTROY_SETTINGS(filename);
  }

  void TabMolecularOpt::readSettings(const QString &filename)
  {
    SETTINGS(filename);

    settings->beginGroup("xtalopt/opt/");

    // Config version
    int loadedVersion = settings->value("version", 0).toInt();

    // Initial generation
    ui.spin_numInitial->setValue(       settings->value("opt/numInitial",       20).toInt()     );

    // Search parameters
    ui.spin_popSize->setValue(          settings->value("opt/popSize",          20).toUInt()    );
    ui.spin_contStructs->setValue(      settings->value("opt/contStructs",      10).toUInt()    );
    ui.cb_limitRunningJobs->setChecked( settings->value("opt/limitRunningJobs"  ,false).toBool());
    ui.spin_runningJobLimit->setValue(  settings->value("opt/runningJobLimit",  1).toUInt()    );
    ui.spin_failLimit->setValue(        settings->value("opt/failLimit",        2).toUInt()    );
    ui.combo_failAction->setCurrentIndex(settings->value("opt/failAction",      XtalOpt::FA_Randomize).toUInt()    );
    ui.spin_cutoff->setValue(           settings->value("opt/cutoff",           100).toInt()    );

    settings->endGroup();

    // Update config data
    switch (loadedVersion) {
    case 0:
    case 1:
    default:
      break;
    }

    updateOptimizationInfo();
  }

  void TabMolecularOpt::updateGUI()
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    // Initial generation
    ui.spin_numInitial->setValue(       xtalopt->numInitial);

    // Search parameters
    ui.spin_popSize->setValue(          xtalopt->popSize);
    ui.spin_contStructs->setValue(      xtalopt->contStructs);
    ui.cb_limitRunningJobs->setChecked( xtalopt->limitRunningJobs);
    ui.spin_runningJobLimit->setValue(  xtalopt->runningJobLimit);
    ui.spin_failLimit->setValue(        xtalopt->failLimit);
    ui.combo_failAction->setCurrentIndex(xtalopt->failAction);
    ui.spin_cutoff->setValue(           xtalopt->cutoff);
  }

  void TabMolecularOpt::lockGUI()
  {
    ui.spin_numInitial->setDisabled(true);
    ui.list_seeds->setDisabled(true);
    ui.push_addSeed->setDisabled(true);
    ui.push_addSeed->setDisabled(true);
    ui.push_removeSeed->setDisabled(true);
  }

  void TabMolecularOpt::updateOptimizationInfo()
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    // Initial generation
    xtalopt->numInitial           = ui.spin_numInitial->value();
    if (int(xtalopt->numInitial) < ui.list_seeds->count())
      ui.spin_numInitial->setValue(ui.list_seeds->count());

    // Search parameters
    xtalopt->popSize              = ui.spin_popSize->value();
    xtalopt->contStructs          = ui.spin_contStructs->value();
    xtalopt->runningJobLimit	= ui.spin_runningJobLimit->value();
    xtalopt->limitRunningJobs	= ui.cb_limitRunningJobs->isChecked();
    xtalopt->failLimit		= ui.spin_failLimit->value();
    xtalopt->failAction		= XtalOpt::FailActions(ui.combo_failAction->currentIndex());
    xtalopt->cutoff              = ui.spin_cutoff->value();
  }

  void TabMolecularOpt::addSeed(QListWidgetItem *item) {
    //qDebug() << "TabMolecularOpt::addSeed( " << item << " ) called";
    QSettings settings;
    QString filename ("");
    bool replace = false;
    if (item) replace = true;

    // Set filename
    if (replace) {
      filename = item->text();
    }
    else {
      filename = settings.value("xtalopt/opt/seedPath", m_opt->filePath + "/POSCAR").toString();
    }

    // Launch file dialog
    QString newFilename = QFileDialog::getOpenFileName(m_dialog,
                        QString("Select structure file to use as seed"),
                        filename,
                        "Common formats (*POSCAR *CONTCAR *.got *.cml *cif"
                        " *.out);;All Files (*)",
                        0, QFileDialog::DontUseNativeDialog);

    // User canceled
    if (newFilename.isEmpty()) return;

    settings.setValue("xtalopt/opt/seedPath", newFilename);

    // Update text
    if (replace)	item->setText(newFilename);
    else		ui.list_seeds->addItem(newFilename);
    updateOptimizationInfo();
    updateSeeds();
  }

  void TabMolecularOpt::removeSeed()
  {
    if (ui.list_seeds->count() == 0) return;
    delete ui.list_seeds->takeItem(ui.list_seeds->currentRow());
    updateSeeds();
  }

  void TabMolecularOpt::updateSeeds()
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    xtalopt->seedList.clear();
    for (int i = 0; i < ui.list_seeds->count(); i++)
      xtalopt->seedList.append(ui.list_seeds->item(i)->text());
  }

}
