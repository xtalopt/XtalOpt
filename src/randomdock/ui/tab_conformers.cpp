/**********************************************************************
  RandomDock -- A tool for analysing a matrix-substrate docking problem

  Copyright (C) 2009 by David Lonie

  This library is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as
  published by the Free Software Foundation; either version 2.1 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include "tab_conformers.h"

#include "randomdock.h"
#include "randomdockdialog.h"
#include "matrixmol.h"
#include "substratemol.h"

#include <openbabel/forcefield.h>

#include <QSettings>
#include <QMessageBox>
#include <QProgressDialog>

using namespace std;

namespace Avogadro {

  TabConformers::TabConformers( RandomDockParams *p ) :
    QObject( p->dialog ), m_params(p), m_ff(0)
  {
    qDebug() << "TabConformers::TabConformers( " << p <<  " ) called.";

    m_tab_widget = new QWidget;
    ui.setupUi(m_tab_widget);

    m_ff = OpenBabel::OBForceField::FindForceField("MMFF94");

    // dialog connections
    connect(p->dialog, SIGNAL(tabsReadSettings()),
            this, SLOT(readSettings()));
    connect(p->dialog, SIGNAL(tabsWriteSettings()),
            this, SLOT(writeSettings()));
    connect(this, SIGNAL(moleculeChanged(Molecule*)),
            p->dialog, SIGNAL(moleculeChanged(Molecule*)));

    // tab connections
    connect(ui.push_generate, SIGNAL(clicked()),
            this, SLOT(generateConformers()));
    connect(ui.push_refresh, SIGNAL(clicked()),
            this, SLOT(updateMoleculeList()));
    connect(ui.combo_mol, SIGNAL(currentIndexChanged(QString)),
            this, SLOT(selectMolecule(QString)));
    connect(ui.table_conformers, SIGNAL(currentCellChanged(int, int, int, int)),
            this, SLOT(conformerChanged(int, int, int, int)));
    connect(ui.cb_allConformers, SIGNAL(toggled(bool)),
            this, SLOT(calculateNumberOfConformers(bool)));
  }

  TabConformers::~TabConformers()
  {
    qDebug() << "TabConformers::~TabConformers() called";
    writeSettings();
  }

  void TabConformers::writeSettings() {
    qDebug() << "TabConformers::writeSettings() called";
    QSettings settings; // Already set up in avogadro/src/main.cpp
    settings.setValue("randomdock/conformers/number",      	ui.spin_nConformers->value());
    settings.setValue("randomdock/conformers/all",      	ui.cb_allConformers->isChecked());
  }

  void TabConformers::readSettings() {
    qDebug() << "TabConformers::readSettings() called";
    QSettings settings; // Already set up in avogadro/src/main.cpp
    ui.spin_nConformers->setValue(	settings.value("randomdock/conformers/number",		100).toInt());
    ui.cb_allConformers->setChecked(	settings.value("randomdock/conformers/all",		true).toInt());
  }

  void TabConformers::updateMoleculeList() {
    qDebug() << "TabConformers::updateMoleculeList() called";
    ui.combo_mol->clear();
    qDebug() << "substrate: " << m_params->substrate;
    if (m_params->substrate)
      ui.combo_mol->addItem("Substrate");
    for (int i = 0; i < m_params->matrixList->size(); i++) {
      ui.combo_mol->addItem(tr("Matrix %1").arg(i+1));
    }
  }

  void TabConformers::generateConformers() {
    qDebug() << "TabConformers::generateConformers() called";
    if (ui.combo_opt->currentIndex() == O_G03) {
      // TODO: implement and use a G03 opt routine...
      return;
    }
    
    /******************************************************
     * Most of this is shamelessly taken from Avogadro's  *
     * forcefield extension...                            *
     ******************************************************/

    OpenBabel::OBForceField *ff = m_ff->MakeNewInstance();
    ff->SetLogFile(&std::cout);
    ff->SetLogLevel(OBFF_LOGLVL_NONE);

    Molecule* mol = currentMolecule();
    OpenBabel::OBMol obmol = mol->OBMol();
    qDebug() << obmol.NumAtoms();
    if (!ff) {
      QMessageBox::warning( m_params->dialog, tr( "Avogadro" ),
                            tr( "Problem setting up forcefield '%1'.")
                            .arg(ui.combo_opt->currentText().trimmed()));
      return;
    }
    if (!ff->Setup(obmol)) {
      QMessageBox::warning( m_params->dialog, tr( "Avogadro" ),
                            tr( "Cannot set up the force field for this molecule." ));
      return;
    }

    // Pre-optimize
    ff->ConjugateGradients(1000);

    // Systematic search:
    if (ui.cb_allConformers->isChecked()) {
      int n = ff->SystematicRotorSearchInitialize(2500);
      QProgressDialog prog ("Performing Systematic conformer search...", 0, 0, n, m_params->dialog);
      int step = 0;
      prog.setValue(step);
      while (ff->SystematicRotorSearchNextConformer(500)) {
        prog.setValue(++step);
        qDebug() << step << " " << n;
      }
    }
    // Random conformer search
    else {
      int n = ui.spin_nConformers->value();
      ff->RandomRotorSearchInitialize(n, 2500);
      QProgressDialog prog ("Performing Random conformer search...", 0, 0, n, m_params->dialog);
      int step = 0;
      prog.setValue(step);
      while (ff->RandomRotorSearchNextConformer(500)) {
        prog.setValue(++step);
        qDebug() << step << " " << n;
      }
    }
    obmol = mol->OBMol();
    // copy conformers ff-->obmol
    ff->GetConformers( obmol );
    // copy conformer obmol-->mol
    std::vector<Eigen::Vector3d> conformer;
    std::vector<std::vector<Eigen::Vector3d>*> confs;
    std::vector<double> energies;

    for (int i = 0; i < obmol.NumConformers(); i++) {
      double *coordPtr = obmol.GetConformer(i);
      conformer.clear();
      foreach (Atom *atom, mol->atoms()) {
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
    //mol->setAllConformers(confs);
    //mol->setEnergies(energies);
    qDebug() << "Lengths : " << confs.size() << " " << energies.size() << " " << mol->numConformers() << " " << mol->energies().size();
    delete ff;
    updateConformerTable();
  }

  void TabConformers::selectMolecule(const QString & text) {
    qDebug() << "TabConformers::selectMolecule( " << text << " ) called";
    Molecule* mol = currentMolecule();
    emit moleculeChanged(mol);
    updateConformerTable();
    calculateNumberOfConformers(ui.cb_allConformers->isChecked());
  }

  void TabConformers::updateConformerTable() {
    qDebug() << "TabConformers::updateConformerTable() called";

    Molecule *mol = currentMolecule();

    // Generate probability lists:
    QList<double> tmp, probs;
    for (uint i = 0; i < mol->numConformers(); i++) {
      if (ui.combo_mol->currentText().contains("Substrate"))
        tmp.append(static_cast<Substrate*>(mol)->prob(i));
      else if (ui.combo_mol->currentText().contains("Matrix"))
        tmp.append(static_cast<Matrix*>(mol)->prob(i));
      else tmp.append(0);
    }
    for (int i = 0; i < tmp.size(); i++)
      if (i == 0) probs.append(tmp.at(i));
      else probs.append(tmp.at(i) - tmp.at(i-1));

    // Clear table
    while (ui.table_conformers->rowCount() != 0)
      ui.table_conformers->removeRow(0);

    // Fill table
    for (uint i = 0; i < mol->numConformers(); i++) {
      ui.table_conformers->insertRow(i);
      ui.table_conformers->setItem(i, Conformer, new QTableWidgetItem(QString::number(i+1)));
      ui.table_conformers->setItem(i, Energy, new QTableWidgetItem(QString::number(mol->energy(i))));
      ui.table_conformers->setItem(i, Prob, new QTableWidgetItem(QString::number(probs.at(i) * 100) + " %"));
    }
  }

  void TabConformers::conformerChanged(int row, int, int oldrow, int) {
    qDebug() << "TabConformers::conformerChanged( " << row << ", " << oldrow << " ) called";
    if (row == oldrow)	return;
    if (row == -1)	{emit moleculeChanged(new Molecule); return;}
    Molecule *mol = currentMolecule();
    mol->setConformer(row);
    emit moleculeChanged(mol);
  }

  Molecule* TabConformers::currentMolecule() {
    qDebug() << "TabConformers::currentMolecule() called";
    Molecule *mol;
    QString text = ui.combo_mol->currentText();
    if (text == "Substrate")
      mol = m_params->substrate;
    else if (text.contains("Matrix")) {
      int ind = text.split(" ")[1].toInt() - 1;
      if (ind + 1 > m_params->matrixList->size())
        mol = new Molecule;
      else
        mol = m_params->matrixList->at(ind);
    }
    else mol = new Molecule;
    return mol;
  }

  void TabConformers::calculateNumberOfConformers(bool isSystematic) {
    qDebug() << "TabConformers::calculateNumberOfConformers( " << isSystematic << " ) called";

    // If we don't want a systematic search, let the user pick the number of conformers
    if (!isSystematic) return;

    // I probably will never add the external conformer search....
    if (ui.combo_opt->currentIndex() == O_G03) return;

    // If there aren't any atoms in the molecule, don't bother checking either.
    if (currentMolecule()->numAtoms() == 0) return;

    OpenBabel::OBForceField *ff = m_ff->MakeNewInstance();
    ff->SetLogFile(&std::cout);
    ff->SetLogLevel(OBFF_LOGLVL_NONE);

    Molecule* mol = currentMolecule();
    OpenBabel::OBMol obmol = mol->OBMol();
    qDebug() << obmol.NumAtoms();
    if (!ff) {
      QMessageBox::warning( m_params->dialog, tr( "Avogadro" ),
                            tr( "Problem setting up forcefield '%1'.")
                            .arg(ui.combo_opt->currentText().trimmed()));
      return;
    }
    if (!ff->Setup(obmol)) {
      QMessageBox::warning( m_params->dialog, tr( "Avogadro" ),
                            tr( "Cannot set up the force field for this molecule." ));
      return;
    }
    int n = ff->SystematicRotorSearchInitialize();
    ui.spin_nConformers->setValue(n);
    delete ff;
  }
}

#include "tab_conformers.moc"
