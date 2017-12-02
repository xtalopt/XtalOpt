/**********************************************************************
  RandomDock -- A tool for analysing a matrix-substrate docking problem

  Copyright (C) 2009-2011 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <randomdock/ui/tab_params.h>

#include <randomdock/randomdock.h>
#include <randomdock/structures/matrix.h>
#include <randomdock/structures/substrate.h>
#include <randomdock/ui/dialog.h>

#include <globalsearch/macros.h>

#include <QDebug>
#include <QSettings>

#include <QFileDialog>
#include <QMessageBox>

using namespace std;
using namespace Avogadro;

namespace RandomDock {

TabParams::TabParams(RandomDockDialog* dialog, RandomDock* opt)
  : AbstractTab(dialog, opt)
{
  ui.setupUi(m_tab_widget);

  // Optimization connections
  connect(ui.spin_numSearches, SIGNAL(valueChanged(int)), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.spin_numMatrixMols, SIGNAL(valueChanged(int)), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.spin_cutoff, SIGNAL(valueChanged(int)), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.spin_IAD_min, SIGNAL(editingFinished()), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.spin_IAD_max, SIGNAL(editingFinished()), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.spin_radius_min, SIGNAL(editingFinished()), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.spin_radius_max, SIGNAL(editingFinished()), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.cb_radius_auto, SIGNAL(toggled(bool)), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.cb_cluster, SIGNAL(toggled(bool)), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.cb_strictHBonds, SIGNAL(toggled(bool)), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.cb_build2DNetwork, SIGNAL(toggled(bool)), this,
          SLOT(updateOptimizationInfo()));
  initialize();
}

TabParams::~TabParams()
{
}

void TabParams::writeSettings(const QString& filename)
{
  SETTINGS(filename);
  RandomDock* randomdock = qobject_cast<RandomDock*>(m_opt);
  settings->beginGroup("randomdock/params");
  const int version = 1;
  settings->setValue("version", version);

  settings->setValue("runningJobLimit", randomdock->runningJobLimit);
  settings->setValue("numMatrixMol", randomdock->numMatrixMol);
  settings->setValue("cutoff", randomdock->cutoff);
  settings->setValue("IAD_min", randomdock->IAD_min);
  settings->setValue("IAD_max", randomdock->IAD_max);
  settings->setValue("radius_min", randomdock->radius_min);
  settings->setValue("radius_max", randomdock->radius_max);
  settings->setValue("radius_auto", randomdock->radius_auto);
  settings->setValue("cluster_mode", randomdock->cluster_mode);
  settings->setValue("strictHBonds", randomdock->strictHBonds);
  settings->setValue("build2DNetwork", randomdock->build2DNetwork);

  settings->endGroup();
  DESTROY_SETTINGS(filename);
}

void TabParams::readSettings(const QString& filename)
{
  SETTINGS(filename);
  RandomDock* randomdock = qobject_cast<RandomDock*>(m_opt);
  settings->beginGroup("randomdock/params");
  int loadedVersion = settings->value("version", 0).toInt();

  ui.spin_numSearches->setValue(settings->value("runningJobLimit", 10).toInt());
  ui.spin_numMatrixMols->setValue(settings->value("numMatrixMol", 1).toInt());
  ui.spin_cutoff->setValue(settings->value("cutoff", 0).toInt());
  ui.spin_IAD_min->setValue(settings->value("IAD_min", 0.8).toDouble());
  ui.spin_IAD_max->setValue(settings->value("IAD_max", 3.0).toDouble());
  ui.spin_radius_min->setValue(settings->value("radius_min", 20).toDouble());
  ui.spin_radius_max->setValue(settings->value("radius_max", 100).toDouble());
  ui.cb_radius_auto->setChecked(settings->value("radius_auto", true).toBool());
  ui.cb_cluster->setChecked(settings->value("cluster_mode", false).toBool());
  ui.cb_strictHBonds->setChecked(
    settings->value("strictHBonds", false).toBool());
  ui.cb_build2DNetwork->setChecked(
    settings->value("build2DNetwork", false).toBool());

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

void TabParams::lockGUI()
{
  ui.spin_numMatrixMols->setDisabled(true);
}

void TabParams::updateOptimizationInfo()
{
  RandomDock* randomdock = qobject_cast<RandomDock*>(m_opt);
  // Logic first!
  if (ui.spin_IAD_min->value() > ui.spin_IAD_max->value())
    ui.spin_IAD_max->setValue(ui.spin_IAD_min->value());
  if (ui.spin_radius_min->value() > ui.spin_radius_max->value())
    ui.spin_radius_max->setValue(ui.spin_radius_min->value());

  randomdock->runningJobLimit = ui.spin_numSearches->value();
  // Number of continuous structures is the same as the running job
  // limit for now:
  randomdock->contStructs = randomdock->runningJobLimit;
  randomdock->numMatrixMol = ui.spin_numMatrixMols->value();
  randomdock->cutoff = ui.spin_cutoff->value();
  randomdock->IAD_min = ui.spin_IAD_min->value();
  randomdock->IAD_max = ui.spin_IAD_max->value();
  //  Auto radius --
  if (ui.cb_radius_auto->isChecked()) {
    // Check that we have substrate and at least one matrix element
    if (!randomdock->substrate || randomdock->matrixList.size() == 0) {
      ui.cb_radius_auto->setChecked(false);
      return;
    }
    // Iterate over all substrate conformers, find the shortest and the largest
    // radii.
    double sub_short, sub_long, tmp;
    Substrate* sub = randomdock->substrate;
    sub_short = sub_long = sub->radius();
    for (uint i = 0; i < sub->numConformers(); i++) {
      sub->setConformer(i);
      sub->updateMolecule();
      tmp = sub->radius();
      if (tmp < sub_short)
        sub_short = tmp;
      if (tmp > sub_long)
        sub_long = tmp;
    }
    //   Iterate over all atoms in matrix elements conformers, find longest
    //   radius of all
    double mat_short, mat_long;
    mat_short = mat_long = randomdock->matrixList.first()->radius();
    for (int m = 0; m < randomdock->matrixList.size(); m++) {
      Matrix* mat = randomdock->matrixList.at(m);
      for (uint i = 0; i < mat->numConformers(); i++) {
        mat->setConformer(i);
        mat->updateMolecule();
        double tmp = mat->radius();
        if (tmp < mat_short)
          mat_short = tmp;
        if (tmp > mat_long)
          mat_long = tmp;
      }
    }
    ui.spin_radius_min->blockSignals(true);
    ui.spin_radius_max->blockSignals(true);
    ui.spin_radius_min->setValue(mat_short);
    ui.spin_radius_max->setValue(mat_long + sub_long + randomdock->IAD_max);
    ui.spin_radius_min->blockSignals(false);
    ui.spin_radius_max->blockSignals(false);
    ui.spin_radius_min->update();
    ui.spin_radius_max->update();
  }
  randomdock->radius_min = ui.spin_radius_min->value();
  randomdock->radius_max = ui.spin_radius_max->value();
  randomdock->radius_auto = ui.cb_radius_auto->isChecked();
  randomdock->cluster_mode = ui.cb_cluster->isChecked();
  randomdock->strictHBonds = ui.cb_strictHBonds->isChecked();
  randomdock->build2DNetwork = ui.cb_build2DNetwork->isChecked();
}
}
