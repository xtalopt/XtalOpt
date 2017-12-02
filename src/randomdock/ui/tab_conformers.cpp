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

#include <randomdock/ui/tab_conformers.h>

#include <randomdock/randomdock.h>
#include <randomdock/structures/matrix.h>
#include <randomdock/structures/substrate.h>
#include <randomdock/ui/dialog.h>

#include <globalsearch/macros.h>
#include <globalsearch/structure.h>

#include <openbabel/forcefield.h>

#include <avogadro/atom.h>
#include <avogadro/molecule.h>

#include <QSettings>
#include <QtConcurrentRun>

#include <QMessageBox>

using namespace std;
using namespace Avogadro;
using namespace GlobalSearch;

namespace RandomDock {

TabConformers::TabConformers(RandomDockDialog* dialog, RandomDock* opt)
  : AbstractTab(dialog, opt), m_ff(0)
{
  ui.setupUi(m_tab_widget);

  // tab connections
  connect(ui.push_generate, SIGNAL(clicked()), this,
          SLOT(generateConformers()));
  connect(ui.combo_mol, SIGNAL(currentIndexChanged(QString)), this,
          SLOT(selectStructure(QString)));
  connect(ui.table_conformers, SIGNAL(currentCellChanged(int, int, int, int)),
          this, SLOT(conformerChanged(int, int, int, int)));
  connect(ui.cb_allConformers, SIGNAL(toggled(bool)), this,
          SLOT(calculateNumberOfConformers(bool)));
  connect(this, SIGNAL(conformersChanged()), this,
          SLOT(updateConformerTable()));
  connect(ui.combo_opt, SIGNAL(currentIndexChanged(const QString&)), this,
          SLOT(updateForceField(const QString&)));
  connect(this, SIGNAL(conformerGenerationStarting()), this,
          SLOT(disableGenerateButton()));
  connect(this, SIGNAL(conformerGenerationDone()), this,
          SLOT(enableGenerateButton()));

  OpenBabel::OBPlugin::ListAsVector("forcefields", "ids", m_forceFieldList);

  fillForceFieldCombo();

  initialize();
}

TabConformers::~TabConformers()
{
}

void TabConformers::writeSettings(const QString& filename)
{
  SETTINGS(filename);
  settings->beginGroup("randomdock/conformers");
  const int version = 1;
  settings->setValue("version", version);

  settings->setValue("number", ui.spin_nConformers->value());
  settings->setValue("all", ui.cb_allConformers->isChecked());
  settings->setValue("ff", ui.combo_opt->currentText());

  settings->endGroup();
  DESTROY_SETTINGS(filename);
}

void TabConformers::readSettings(const QString& filename)
{
  SETTINGS(filename);
  settings->beginGroup("randomdock/conformers");
  int loadedVersion = settings->value("version", 0).toInt();

  ui.spin_nConformers->setValue(settings->value("number", 100).toInt());
  ui.cb_allConformers->setChecked(settings->value("all", true).toBool());
  updateForceField(settings->value("ff", "").toString());

  settings->endGroup();

  // Update config data
  switch (loadedVersion) {
    case 0:
    case 1:
    default:
      break;
  }
}

void TabConformers::updateGUI()
{
  updateStructureList();
}

void TabConformers::lockGUI()
{
  ui.push_generate->setDisabled(true);
  ui.combo_opt->setDisabled(true);
  ui.spin_nConformers->setDisabled(true);
  ui.cb_allConformers->setDisabled(true);
}

void TabConformers::fillForceFieldCombo()
{
  m_ffMutex.lock();
  ui.combo_opt->blockSignals(true);
  ui.combo_opt->clear();
  for (int i = 0; i < m_forceFieldList.size(); i++) {
    ui.combo_opt->addItem(m_forceFieldList.at(i).c_str());
  }
  ui.combo_opt->blockSignals(false);
  m_ffMutex.unlock();
  updateForceField();
}

void TabConformers::updateStructureList()
{
  RandomDock* randomdock = qobject_cast<RandomDock*>(m_opt);
  ui.combo_mol->blockSignals(true);
  ui.combo_mol->clear();
  if (randomdock->substrate)
    ui.combo_mol->addItem("Substrate");
  for (int i = 0; i < randomdock->matrixList.size(); i++) {
    ui.combo_mol->addItem(tr("Matrix %1").arg(i + 1));
  }
  ui.combo_mol->blockSignals(false);
  ui.combo_mol->setCurrentIndex(0);
  updateConformerTable();
}

void TabConformers::updateForceField(const QString& s)
{
  if (s.isEmpty()) {
    ui.combo_opt->setCurrentIndex(0);
  }

  m_ffMutex.lock();
  // Sync GUI
  ui.combo_opt->blockSignals(true);
  ui.combo_opt->setCurrentIndex(ui.combo_opt->findText(s));
  ui.combo_opt->blockSignals(false);
  // Grab forcefield
  m_ff = OpenBabel::OBForceField::FindForceField(s.toStdString());
  m_ffMutex.unlock();
}

void TabConformers::generateConformers()
{
  emit conformerGenerationStarting();
  QtConcurrent::run(this, &TabConformers::generateConformers_,
                    currentStructure());
}

void TabConformers::generateConformers_(Structure* mol)
{
  m_dialog->startProgressUpdate("Preparing conformer search...", 0, 0);
  QWriteLocker locker(mol->lock());
  QMutexLocker fflocker(&m_ffMutex);

  if (!m_ff) {
    QMessageBox::warning(m_dialog, tr("Avogadro"),
                         tr("Problem setting up forcefield '%1'.")
                           .arg(ui.combo_opt->currentText().trimmed()));
    m_dialog->stopProgressUpdate();
    ui.push_generate->setEnabled(true);
    return;
  }

  m_dialog->updateProgressLabel("Preparing forcefield...");
  OpenBabel::OBForceField* ff = m_ff->MakeNewInstance();
  ff->SetLogFile(&std::cout);
  ff->SetLogLevel(OBFF_LOGLVL_NONE);

  m_dialog->updateProgressLabel("Setting up molecule with forcefield...");
  OpenBabel::OBMol obmol = mol->OBMol();

  // Explicitly set the energy, otherwise the ff may crash due to OB bug
  std::vector<double> obenergies(mol->energies());
  if (obenergies.size() == 0) {
    obenergies.push_back(0.0);
  }
  obmol.SetEnergies(obenergies);

  if (!ff) {
    QMessageBox::warning(m_dialog, tr("Avogadro"),
                         tr("Problem setting up forcefield '%1'.")
                           .arg(ui.combo_opt->currentText().trimmed()));
    m_dialog->stopProgressUpdate();
    ui.push_generate->setEnabled(true);
    return;
  }
  if (!ff->Setup(obmol)) {
    QMessageBox::warning(
      m_dialog, tr("Avogadro"),
      tr("Cannot set up the force field for this molecule."));
    m_dialog->stopProgressUpdate();
    ui.push_generate->setEnabled(true);
    return;
  }

  // Pre-optimize
  m_dialog->updateProgressLabel("Pre-optimizing molecule...");
  ff->ConjugateGradients(1000);

  // Prepare progress step variable
  int step = 0;

  // Systematic search:
  if (ui.cb_allConformers->isChecked()) {
    m_dialog->updateProgressLabel(tr("Performing systematic rotor search..."));
    // Only search if there is more than one conformer possible.
    if (int n = ff->SystematicRotorSearchInitialize(2500) > 1) {
      m_dialog->updateProgressMaximum(2 * n);
      while (ff->SystematicRotorSearchNextConformer(500)) {
        m_dialog->updateProgressValue(++step);
      }
    }
  }
  // Random conformer search
  else {
    int n = ui.spin_nConformers->value();
    m_dialog->updateProgressLabel(tr("Performing random rotor search..."));
    ff->RandomRotorSearchInitialize(n, 2500);
    m_dialog->updateProgressMaximum(2 * n);
    while (ff->RandomRotorSearchNextConformer(500)) {
      m_dialog->updateProgressValue(++step);
    }
  }
  obmol = mol->OBMol();
  // copy conformers ff-->obmol
  ff->GetConformers(obmol);
  // copy conformer obmol-->mol
  std::vector<Eigen::Vector3d> conformer;
  std::vector<std::vector<Eigen::Vector3d>*> confs;
  std::vector<double> energies;

  for (int i = 0; i < obmol.NumConformers(); i++) {
    m_dialog->updateProgressValue(++step);
    double* coordPtr = obmol.GetConformer(i);
    conformer.clear();
    foreach (Atom* atom, mol->atoms()) {
      while (conformer.size() < atom->id())
        conformer.push_back(Eigen::Vector3d(0.0, 0.0, 0.0));
      conformer.push_back(Eigen::Vector3d(coordPtr));
      coordPtr += 3;
    }
    mol->addConformer(conformer, i);
    mol->setEnergy(i, obmol.GetEnergy(i));
    confs.push_back(&conformer);
    energies.push_back(obmol.GetEnergy(i));
  }
  delete ff;
  emit conformersChanged();
  emit conformerGenerationDone();
  m_dialog->stopProgressUpdate();
}

void TabConformers::selectStructure(const QString& text)
{
  Structure* mol = currentStructure();

  if (!mol)
    return;

  updateConformerTable();
  calculateNumberOfConformers(ui.cb_allConformers->isChecked());
  emit moleculeChanged(mol);
}

void TabConformers::updateConformerTable()
{
  Structure* mol = currentStructure();
  if (!mol)
    return;
  QReadLocker locker(mol->lock());

  // Generate probability lists:
  // TODO this can be cleaned up a lot once sub/mat are abstracted
  QList<double> tmp, probs;
  for (uint i = 0; i < mol->numConformers(); i++) {
    if (ui.combo_mol->currentText().contains("Substrate"))
      tmp.append(qobject_cast<Substrate*>(mol)->prob(i));
    else if (ui.combo_mol->currentText().contains("Matrix"))
      tmp.append(qobject_cast<Matrix*>(mol)->prob(i));
    else
      tmp.append(0);
  }
  for (int i = 0; i < tmp.size(); i++)
    if (i == 0)
      probs.append(tmp.at(i));
    else
      probs.append(tmp.at(i) - tmp.at(i - 1));

  // Clear table
  while (ui.table_conformers->rowCount() != 0)
    ui.table_conformers->removeRow(0);

  // Fill table
  for (uint i = 0; i < mol->numConformers(); i++) {
    ui.table_conformers->insertRow(i);
    ui.table_conformers->setItem(i, Conformer,
                                 new QTableWidgetItem(QString::number(i + 1)));
    ui.table_conformers->setItem(
      i, Energy, new QTableWidgetItem(QString::number(mol->energy(i))));
    ui.table_conformers->setItem(
      i, Prob, new QTableWidgetItem(QString::number(probs.at(i) * 100) + " %"));
  }
}

void TabConformers::conformerChanged(int row, int, int oldrow, int)
{
  if (row == oldrow)
    return;
  if (row == -1) {
    emit moleculeChanged(new Structure);
    return;
  }
  Structure* mol = currentStructure();
  if (!mol)
    return;
  mol->setConformer(row);
  emit moleculeChanged(mol);
}

Structure* TabConformers::currentStructure()
{
  QString text = ui.combo_mol->currentText();
  if (text.isEmpty())
    return 0;

  RandomDock* randomdock = qobject_cast<RandomDock*>(m_opt);
  Structure* mol;
  if (text == "Substrate")
    mol = qobject_cast<Structure*>(randomdock->substrate);
  else if (text.contains("Matrix")) {
    int ind = text.split(" ")[1].toInt() - 1;
    if (ind + 1 > randomdock->matrixList.size())
      mol = new Structure;
    else
      mol = qobject_cast<Structure*>(randomdock->matrixList.at(ind));
  } else
    mol = new Structure;
  return mol;
}

void TabConformers::calculateNumberOfConformers(bool isSystematic)
{
  // If we don't want a systematic search, let the user pick the
  // number of conformers
  if (!isSystematic)
    return;

  // If there aren't any atoms in the structure, don't bother
  // checking either.
  Structure* mol = currentStructure();
  if (!mol || mol->numAtoms() == 0)
    return;

  QMutexLocker ffLocker(&m_ffMutex);

  if (!m_ff) {
    QMessageBox::warning(m_dialog, tr("Avogadro"),
                         tr("Problem setting up forcefield '%1'.")
                           .arg(ui.combo_opt->currentText().trimmed()));
    m_dialog->stopProgressUpdate();
    ui.push_generate->setEnabled(true);
    return;
  }

  OpenBabel::OBForceField* ff = m_ff->MakeNewInstance();
  ff->SetLogFile(&std::cout);
  ff->SetLogLevel(OBFF_LOGLVL_NONE);

  QReadLocker locker(mol->lock());
  OpenBabel::OBMol obmol = mol->OBMol();
  if (!ff) {
    QMessageBox::warning(m_dialog, tr("Avogadro"),
                         tr("Problem setting up forcefield '%1'.")
                           .arg(ui.combo_opt->currentText().trimmed()));
    return;
  }
  if (!ff->Setup(obmol)) {
    QMessageBox::warning(
      m_dialog, tr("Avogadro"),
      tr("Cannot set up the force field for this molecule."));
    return;
  }
  int n = ff->SystematicRotorSearchInitialize();
  ui.spin_nConformers->setValue(n);
  delete ff;
}
}
