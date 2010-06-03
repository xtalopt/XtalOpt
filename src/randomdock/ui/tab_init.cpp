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

#include "tab_init.h"

#include "dialog.h"
#include "../randomdock.h"
#include "../structures/substrate.h"
#include "../structures/matrix.h"
#include "../../generic/macros.h"

#include <avogadro/moleculefile.h>

#include <QSettings>
#include <QFileDialog>

using namespace std;
using namespace Avogadro;

namespace RandomDock {

  TabInit::TabInit( RandomDockDialog *dialog, RandomDock *opt ) :
    QObject( dialog ),
    m_dialog(dialog),
    m_opt(opt)
  {
    m_tab_widget = new QWidget;
    ui.setupUi(m_tab_widget);

    // dialog connections
    connect(m_dialog, SIGNAL(tabsReadSettings(const QString &)),
            this, SLOT(readSettings(const QString &)));
    connect(m_dialog, SIGNAL(tabsWriteSettings(const QString &)),
            this, SLOT(writeSettings(const QString &)));
    connect(m_dialog, SIGNAL(tabsUpdateGUI()),
            this, SLOT(updateGUI()));
    connect(m_dialog, SIGNAL(tabsDisconnectGUI()),
            this, SLOT(disconnectGUI()));
    connect(m_dialog, SIGNAL(tabsLockGUI()),
            this, SLOT(lockGUI()));

    // tab connections
    connect(ui.edit_substrateFile, SIGNAL(textChanged(QString)),
            this, SLOT(updateParams()));
    connect(ui.push_substrateBrowse, SIGNAL(clicked()),
            this, SLOT(substrateBrowse()));
    connect(ui.push_substrateCurrent, SIGNAL(clicked()),
            this, SLOT(substrateCurrent()));
    connect(ui.push_matrixAdd, SIGNAL(clicked()),
            this, SLOT(matrixAdd()));
    connect(ui.push_matrixRemove, SIGNAL(clicked()),
            this, SLOT(matrixRemove()));
    connect(ui.push_matrixCurrent, SIGNAL(clicked()),
            this, SLOT(matrixCurrent()));
    connect(ui.table_matrix, SIGNAL(itemChanged(QTableWidgetItem*)),
            this, SLOT(updateParams()));
    connect(ui.push_readFiles, SIGNAL(clicked()),
            this, SLOT(readFiles()));
  }

  TabInit::~TabInit()
  {
    qDebug() << "TabInit::~TabInit() called";
    writeSettings();
  }

  void TabInit::writeSettings(const QString &filename)
  {
    SETTINGS(filename);
    settings->beginGroup("randomdock/init");

    settings->endGroup();
    DESTROY_SETTINGS(filename);
  }

  void TabInit::readSettings(const QString &filename)
  {
    SETTINGS(filename);
    settings->beginGroup("randomdock/init");

    settings->endGroup();      
  }

  void TabInit::updateGUI()
  {
  }

  void TabInit::disconnectGUI()
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
    ui.push_readFiles->setDisabled(true);
    ui.table_matrix->setDisabled(true);
  }

  void TabInit::updateParams() {
    qDebug() << "TabInit::updateParams() called";
    m_opt->substrateFile = ui.edit_substrateFile->text();
    m_opt->matrixFiles.clear();
    m_opt->matrixStoich.clear();
    for (int i = 0; i < ui.table_matrix->rowCount(); i++) {
      ui.table_matrix->item(i, Num)->setText(QString::number(i+1));
      m_opt->matrixFiles.append(ui.table_matrix->item(i, Filename)->text());
      m_opt->matrixStoich.append(ui.table_matrix->item(i, Stoich)->text().toInt());
    }
  }

  void TabInit::substrateBrowse() {
    qDebug() << "TabInit::substrateBrowse() called";
    QSettings settings;
    QString path = settings.value("randomdock/paths/substrateBrowse", "").toString();
    QString fileName = QFileDialog::getOpenFileName(m_dialog, 
                                                    tr("Select molecule file to use for the substrate"),
                                                    path,
                                                    tr("All files (*)"));
    settings.setValue("randomdock/paths/substrateBrowse", fileName);
    ui.edit_substrateFile->setText(fileName);
  }

  void TabInit::substrateCurrent() {
    qDebug() << "TabInit::substrateCurrent() called";
  }

  void TabInit::matrixAdd() {
    qDebug() << "TabInit::matrixAdd() called";
    QSettings settings;
    QString path = settings.value("randomdock/paths/matrixBrowse", "").toString();
    QString fileName = QFileDialog::getOpenFileName(m_dialog, 
                                                    tr("Select molecule file to add as a matrix element"),
                                                    path,
                                                    tr("All files (*)"));
    settings.setValue("randomdock/paths/matrixBrowse", fileName);

    int row = ui.table_matrix->rowCount();
    ui.table_matrix->insertRow(row);
    // Block signals for all but the last addition -- this keeps
    // updateParams() from crashing by not reading null items.
    ui.table_matrix->blockSignals(true);
    ui.table_matrix->setItem(row, Num, new QTableWidgetItem(QString::number(row+1)));
    ui.table_matrix->setItem(row, Filename, new QTableWidgetItem(fileName));
    ui.table_matrix->blockSignals(false);
    ui.table_matrix->setItem(row, Stoich, new QTableWidgetItem(QString::number(1)));
  }

  void TabInit::matrixRemove() {
    qDebug() << "TabInit::matrixRemove() called";
    ui.table_matrix->removeRow(ui.table_matrix->currentRow());
    updateParams();
  }

  void TabInit::matrixCurrent() {
    qDebug() << "TabInit::matrixCurrent() called";
  }

  void TabInit::readFiles() {
    qDebug() << "TabInit::readFiles() called";

    QApplication::setOverrideCursor( Qt::WaitCursor );
    Molecule *mol;

    // Substrate
    if (m_opt->substrate) {
      m_opt->substrate = 0;
    }
    qDebug() << m_opt->substrateFile;
    if (!m_opt->substrateFile.isEmpty()) {
      mol = MoleculeFile::readMolecule(m_opt->substrateFile);
      m_opt->substrate = new Substrate (mol);
      qDebug() << "Updated substrate: " << m_opt->substrate << " #atoms= " << m_opt->substrate->numAtoms();
    }

    // Matrix
    m_opt->matrixList.clear();
    for (int i = 0; i < m_opt->matrixFiles.size(); i++) {
      qDebug() << m_opt->matrixFiles.at(i);
      mol = MoleculeFile::readMolecule(m_opt->matrixFiles.at(i));
      m_opt->matrixList.append(new Matrix (mol));
      qDebug() << "Matrix added:" << m_opt->matrixList.at(i);
    }
    QApplication::restoreOverrideCursor();
  }

}

//#include "tab_init.moc"
