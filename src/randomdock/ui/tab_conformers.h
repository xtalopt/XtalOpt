/**********************************************************************
  RandomDock -- A tool for analysing a matrix-substrate docking problem

  Copyright (C) 2009-2011 by David Lonie

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

#include <QtCore/QMutex>

namespace GlobalSearch {
  class Structure;
}

namespace Avogadro {
  class Molecule;
}

namespace OpenBabel {
  class OBForceField;
}

namespace RandomDock {
  class RandomDockDialog;
  class RandomDock;

  class TabConformers : public GlobalSearch::AbstractTab
  {
    Q_OBJECT

  public:
    explicit TabConformers( RandomDockDialog *dialog, RandomDock *opt);
    virtual ~TabConformers();

    enum Columns {
      Conformer = 0,
      Energy,
      Prob
    };

    GlobalSearch::Structure* currentStructure();

  public slots:
    void lockGUI();
    void readSettings(const QString &filename = "");
    void writeSettings(const QString &filename = "");
    void updateGUI();
    void generateConformers();
    void fillForceFieldCombo();
    void updateStructureList();
    void updateConformerTable();
    void updateForceField(const QString & s = "");
    void selectStructure(const QString & text);
    void conformerChanged(int row, int, int oldrow, int);
    void calculateNumberOfConformers(bool isSystematic);

  signals:
    void conformerGenerationStarting();
    void conformerGenerationDone();
    void conformersChanged();

  private slots:
    void enableGenerateButton() {ui.push_generate->setEnabled(true);};
    void disableGenerateButton() {ui.push_generate->setEnabled(false);};

  private:
    Ui::Tab_Conformers ui;
    OpenBabel::OBForceField *m_ff;
    std::vector<std::string> m_forceFieldList;
    QMutex m_ffMutex;

    void generateConformers_(GlobalSearch::Structure *mol);
  };
}

#endif
