/**********************************************************************
  ConformerGeneratorDialog -- Generate Conformers with RDKit

  Copyright (C) 2017 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifdef ENABLE_MOLECULAR

// Doxygen skip:
/// @cond

#include <QFileDialog>

#include <globalsearch/optbase.h>
#include <globalsearch/molecular/conformergenerator.h>

#include "conformergeneratordialog.h"
#include "ui_conformergeneratordialog.h"

namespace GlobalSearch {

  ConformerGeneratorDialog::ConformerGeneratorDialog(QDialog* parent,
                                                     OptBase* o)
    : QDialog(parent),
      m_opt(o),
      ui(new Ui::ConformerGeneratorDialog)
  {
    ui->setupUi(this);

    connect(ui->push_browseInitialMolFile, SIGNAL(clicked()),
            this, SLOT(browseInitialMolFile()));
    connect(ui->push_browseConformerOutDir, SIGNAL(clicked()),
            this, SLOT(browseConformerOutDir()));

    updateGUI();
  }

  ConformerGeneratorDialog::~ConformerGeneratorDialog()
  {
    delete ui;
  }

  void ConformerGeneratorDialog::updateGUI()
  {
    ui->edit_initialMolFile->blockSignals(true);
    ui->edit_conformerOutDir->blockSignals(true);
    ui->spin_numConfs->blockSignals(true);
    ui->spin_rmsdThreshold->blockSignals(true);
    ui->cb_mmffOptConfs->blockSignals(true);
    ui->spin_maxOptIters->blockSignals(true);
    ui->cb_pruneConfsAfterOpt->blockSignals(true);

    ui->edit_initialMolFile->setText(m_opt->m_initialMolFile.c_str());
    ui->edit_conformerOutDir->setText(m_opt->m_conformerOutDir.c_str());
    ui->spin_numConfs->setValue(m_opt->m_numConformersToGenerate);
    ui->spin_rmsdThreshold->setValue(m_opt->m_rmsdThreshold);
    ui->cb_mmffOptConfs->setChecked(m_opt->m_mmffOptConfs);
    ui->spin_maxOptIters->setValue(m_opt->m_maxOptIters);
    ui->cb_pruneConfsAfterOpt->setChecked(m_opt->m_pruneConfsAfterOpt);

    // Set these enabled or disabled depending on the settings
    ui->label_maxOptIters->setEnabled(m_opt->m_mmffOptConfs);
    ui->spin_maxOptIters->setEnabled(m_opt->m_mmffOptConfs);
    ui->cb_pruneConfsAfterOpt->setEnabled(m_opt->m_mmffOptConfs);

    ui->edit_initialMolFile->blockSignals(false);
    ui->edit_conformerOutDir->blockSignals(false);
    ui->spin_numConfs->blockSignals(false);
    ui->spin_rmsdThreshold->blockSignals(false);
    ui->cb_mmffOptConfs->blockSignals(false);
    ui->spin_maxOptIters->blockSignals(false);
    ui->cb_pruneConfsAfterOpt->blockSignals(false);
  }

  void ConformerGeneratorDialog::accept()
  {
    // We need to end the output directory with '/'
    QString confOutDir = ui->edit_conformerOutDir->text();
    if (!confOutDir.endsWith(QDir::separator()))
      confOutDir.append(QDir::separator());

    m_opt->m_initialMolFile = ui->edit_initialMolFile->text().toStdString();
    m_opt->m_conformerOutDir = confOutDir.toStdString();
    m_opt->m_numConformersToGenerate = ui->spin_numConfs->value();
    m_opt->m_rmsdThreshold = ui->spin_rmsdThreshold->value();
    m_opt->m_mmffOptConfs = ui->cb_mmffOptConfs->isChecked();
    m_opt->m_maxOptIters = ui->spin_maxOptIters->value();
    m_opt->m_pruneConfsAfterOpt = ui->cb_pruneConfsAfterOpt->isChecked();

    if (!m_opt->generateConformers()) {
      m_opt->error(tr("Failed to generate conformers. Check terminal "
                      "output for details\n"));
    }

    QDialog::accepted();
    close();
  }

  void ConformerGeneratorDialog::reject()
  {
    QDialog::reject();
    close();
  }

  void ConformerGeneratorDialog::browseInitialMolFile()
  {
    QString oldFile = m_opt->m_initialMolFile.c_str();
    QString newFile = QFileDialog::getOpenFileName(
                         this,
                         tr("Select Initial Conformer file (pdb format)..."),
                         oldFile, "*.pdb;;*.*",
                         nullptr, QFileDialog::DontUseNativeDialog);

    if (!newFile.isEmpty()) {
      m_opt->m_initialMolFile = newFile.toStdString();
      updateGUI();
    }
  }

  void ConformerGeneratorDialog::browseConformerOutDir()
  {
    QString oldDir = m_opt->m_conformerOutDir.c_str();
    QString newDir = QFileDialog::getExistingDirectory(
                       this,
                       tr("Select the directory to put the conformers in..."),
                       oldDir);

    if (!newDir.endsWith(QDir::separator()))
      newDir.append(QDir::separator());

    if (!newDir.isEmpty()) {
      m_opt->m_conformerOutDir = newDir.toStdString();
      updateGUI();
    }
  }
}

/// @endcond
#endif // ENABLE_MOLECULAR
