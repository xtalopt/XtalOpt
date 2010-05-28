/**********************************************************************
  RandomDock -- A tool for analysing a matrix-substrate docking problem

  Copyright (C) 2009 by David Lonie

  This file is part of the Avogadro molecular editor project.
  For more information, see <http://avogadro.openmolecules.net/>

  This library is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as
  published by the Free Software Foundation; either version 2.1 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public icense for more details.
 ***********************************************************************/

#ifndef TAB_CONFORMERS_H
#define TAB_CONFORMERS_H

#include "ui_tab_conformers.h"

namespace OpenBabel {
  class OBForceField;
}

namespace Avogadro {
  class Molecule;
}

using namespace Avogadro;

namespace RandomDock {
  class RandomDockDialog;
  class RandomDock;

  class TabConformers : public QObject
  {
    Q_OBJECT

  public:
    explicit TabConformers( RandomDockDialog *dialog, RandomDock *opt);
    virtual ~TabConformers();

    enum OptTypes	{O_G03 = 0, O_Ghemical, O_MMFF94, O_MMFF94s, O_UFF};
    enum Columns	{Conformer = 0, Energy, Prob};

    QWidget *getTabWidget() {return m_tab_widget;};
    Molecule* currentMolecule();

  public slots:
    void readSettings();
    void writeSettings();
    void generateConformers();
    void optimizeConformers();
    void updateMoleculeList();
    void updateConformerTable();
    void selectMolecule(const QString & text);
    void conformerChanged(int row, int, int oldrow, int);
    void calculateNumberOfConformers(bool isSystematic);

  signals:
    void moleculeChanged(Molecule*);

  private:
    Ui::Tab_Conformers ui;
    QWidget *m_tab_widget;
    Molecule *m_molecule;
    OpenBabel::OBForceField *m_ff;
    RandomDockDialog *m_dialog;
    RandomDock *m_opt;
  };
}

#endif
