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

#include <randomdock/ui/tab_init.h>

#include <globalsearch/macros.h>
#include <randomdock/randomdock.h>
#include <randomdock/structures/matrix.h>
#include <randomdock/structures/substrate.h>
#include <randomdock/ui/dialog.h>

#include <avogadro/moleculefile.h>

#include <QSettings>

#include <QFileDialog>
#include <QMessageBox>

using namespace std;
using namespace Avogadro;

namespace RandomDock {

TabInit::TabInit(RandomDockDialog* dialog, RandomDock* opt)
  : AbstractTab(dialog, opt)
{
  ui.setupUi(m_tab_widget);

  // tab connections
  connect(ui.edit_substrateFile, SIGNAL(textChanged(QString)), this,
          SLOT(updateParams()));
  connect(ui.push_substrateBrowse, SIGNAL(clicked()), this,
          SLOT(substrateBrowse()));
  connect(ui.push_substrateCurrent, SIGNAL(clicked()), this,
          SLOT(substrateCurrent()));
  connect(ui.push_matrixAdd, SIGNAL(clicked()), this, SLOT(matrixAdd()));
  connect(ui.push_matrixRemove, SIGNAL(clicked()), this, SLOT(matrixRemove()));
  connect(ui.push_matrixCurrent, SIGNAL(clicked()), this,
          SLOT(matrixCurrent()));
  connect(ui.table_matrix, SIGNAL(itemChanged(QTableWidgetItem*)), this,
          SLOT(updateParams()));

  initialize();
}

TabInit::~TabInit()
{
}

void TabInit::lockGUI()
{
  ui.edit_substrateFile->setDisabled(true);
  ui.push_substrateBrowse->setDisabled(true);
  ui.push_substrateCurrent->setDisabled(true);
  ui.push_matrixAdd->setDisabled(true);
  ui.push_matrixCurrent->setDisabled(true);
  ui.push_matrixRemove->setDisabled(true);
  ui.table_matrix->setDisabled(true);
}

void TabInit::updateParams()
{
  RandomDock* randomdock = qobject_cast<RandomDock*>(m_opt);
  randomdock->substrateFile = ui.edit_substrateFile->text();
  randomdock->matrixFiles.clear();
  randomdock->matrixStoich.clear();
  for (int i = 0; i < ui.table_matrix->rowCount(); i++) {
    ui.table_matrix->item(i, Num)->setText(QString::number(i + 1));
    randomdock->matrixFiles.append(ui.table_matrix->item(i, Filename)->text());
    randomdock->matrixStoich.append(
      ui.table_matrix->item(i, Stoich)->text().toInt());
  }
}

void TabInit::substrateBrowse()
{
  RandomDock* randomdock = qobject_cast<RandomDock*>(m_opt);

  // Initialize with previously selected substrate
  QSettings settings;
  QString path =
    settings.value("randomdock/paths/substrateBrowse", "").toString();
  QString filename;

  // Prompt for file
  bool selectionOk = false;
  Molecule* mol;
  while (!selectionOk) {
    filename = QFileDialog::getOpenFileName(
      m_dialog, tr("Select molecule file to use for the substrate"), path,
      tr("All files (*)"), 0, QFileDialog::DontUseNativeDialog);

    // User cancels
    if (filename.isNull()) {
      return;
    }

    // Read file
    QApplication::setOverrideCursor(Qt::WaitCursor);
    mol = MoleculeFile::readMolecule(filename);
    // Check that molecule loaded successfully
    if (!mol) {
      // Pop-up error
      QApplication::restoreOverrideCursor();
      QMessageBox::warning(m_dialog, tr("Error loading substrate"),
                           tr("Cannot load file %1 for substrate. Check that "
                              "it contains valid molecule information.")
                             .arg(filename));
      continue;
    } else { // molecule loaded successfully
      selectionOk = true;
    }
  }
  // Read ok
  // Delete old substrate if needed
  if (randomdock->substrate) {
    delete randomdock->substrate;
    randomdock->substrate = 0;
  }
  randomdock->substrate = new Substrate(mol);
  delete mol;
  ui.edit_substrateFile->setText(filename);
  QApplication::restoreOverrideCursor();
  settings.setValue("randomdock/paths/substrateBrowse", filename);
  emit substrateChanged(randomdock->substrate);
}

void TabInit::substrateCurrent()
{
}

void TabInit::matrixAdd()
{
  RandomDock* randomdock = qobject_cast<RandomDock*>(m_opt);

  // Initialize filename from settings object
  QSettings settings;
  QString path = settings.value("randomdock/paths/matrixBrowse", "").toString();
  QString filename;

  // Prompt for file
  bool selectionOk = false;
  Molecule* mol;
  while (!selectionOk) {
    filename = QFileDialog::getOpenFileName(
      m_dialog, tr("Select molecule file to add as a matrix element"), path,
      tr("All files (*)"), 0, QFileDialog::DontUseNativeDialog);

    // User cancels
    if (filename.isNull()) {
      return;
    }

    // Read file
    QApplication::setOverrideCursor(Qt::WaitCursor);
    mol = MoleculeFile::readMolecule(filename);
    // Check that molecule loaded successfully
    if (!mol) {
      // Pop-up error
      QApplication::restoreOverrideCursor();
      QMessageBox::warning(m_dialog, tr("Error loading matrix"),
                           tr("Cannot load file %1 as a matrix element. Check "
                              "that it contains valid molecule information.")
                             .arg(filename));
      continue;
    } else { // molecule loaded successfully
      selectionOk = true;
    }
  }
  // Read ok
  Matrix* mat = new Matrix(mol);
  randomdock->matrixList.append(mat);
  randomdock->matrixFiles.append(filename);
  delete mol;
  QApplication::restoreOverrideCursor();
  settings.setValue("randomdock/paths/matrixBrowse", filename);

  int row = ui.table_matrix->rowCount();
  ui.table_matrix->insertRow(row);

  ui.table_matrix->blockSignals(true);
  ui.table_matrix->setItem(row, Num,
                           new QTableWidgetItem(QString::number(row + 1)));
  ui.table_matrix->setItem(row, Filename, new QTableWidgetItem(filename));
  ui.table_matrix->setItem(row, Stoich,
                           new QTableWidgetItem(QString::number(1)));
  ui.table_matrix->blockSignals(false);
  updateParams();
  emit matrixAdded(mat);
}

void TabInit::matrixRemove()
{
  RandomDock* randomdock = qobject_cast<RandomDock*>(m_opt);

  int row = ui.table_matrix->currentRow();
  ui.table_matrix->removeRow(row);
  randomdock->matrixFiles.removeAt(row);
  Matrix* mat = randomdock->matrixList.at(row);
  randomdock->matrixList.removeAt(row);
  delete mat;
  updateParams();
  emit matrixRemoved();
}

void TabInit::matrixCurrent()
{
}
}
