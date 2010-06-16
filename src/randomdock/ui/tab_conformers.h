/**********************************************************************
  RandomDock -- A tool for analysing a matrix-substrate docking problem

  Copyright (C) 2009-2010 by David Lonie

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

#include <globalsearch/ui/abstracttab.h>

#include "ui_tab_conformers.h"

namespace GlobalSearch {
  class Structure;
}

namespace Avogadro {
  class Molecule;
}

namespace OpenBabel {
  class OBForceField;
}

using namespace GlobalSearch;
using namespace Avogadro;
using namespace OpenBabel;

namespace RandomDock {
  class RandomDockDialog;
  class RandomDock;

  class TabConformers : public AbstractTab
  {
    Q_OBJECT

  public:
    explicit TabConformers( RandomDockDialog *dialog, RandomDock *opt);
    virtual ~TabConformers();

    enum OptTypes {
      O_G03 = 0,
      O_Ghemical,
      O_MMFF94,
      O_MMFF94s,
      O_UFF
    };

    enum Columns {
      Conformer = 0,
      Energy,
      Prob
    };

    Structure* currentStructure();

  public slots:
    void lockGUI();
    void readSettings(const QString &filename = "");
    void writeSettings(const QString &filename = "");
    void updateGUI();
    void generateConformers();
    void updateStructureList();
    void updateConformerTable();
    void selectStructure(const QString & text);
    void conformerChanged(int row, int, int oldrow, int);
    void calculateNumberOfConformers(bool isSystematic);

  signals:
    void conformersChanged();

  private:
    Ui::Tab_Conformers ui;
    OBForceField *m_ff;

    void generateConformers_(Structure *mol);
  };
}

#endif
