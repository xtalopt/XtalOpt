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

#include "templates.h"

#include "scenemol.h"
#include "matrixmol.h"
#include "substratemol.h"

#include <openbabel/obiter.h>
#include <openbabel/mol.h>

#include <QDebug>
#include <QFile>
#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>

using namespace OpenBabel;
using namespace std;

namespace Avogadro {

  void Templates::showHelp() {
    qDebug() << "Templates::showHelp() called";

    QString str;
    QTextStream out (&str);
    out 
      << "The following keywords should be used instead of the indicated variable data:\n\n"
      << "%coords% -- cartesian coordinate data\n\t[symbol] [x] [y] [z]\n"
      << "%coordsId% -- cartesian coordinate data with atomic number\n\t[symbol] [atomic number] [x] [y] [z]\n"
      << "%numAtoms% -- Number of atoms in molecule/scene\n"
      << "%filename% -- local output filename\n"
      << "%rempath% -- path to molecule/scene's remote directory\n"
      << "%id% -- scene/conformer id number\n";

    QMessageBox::information(NULL, "Template Help", str);
  }

  QString Templates::interpretTemplate(const QString & str, Scene *scene) {
    qDebug() << "Templates::interpretTemplate( " << "<string omitted>" << ", " << scene << " ) called";

    QStringList list = str.split("%");
    QString line;
    QString rep;
    for (int line_ind = 0; line_ind < list.size(); line_ind++) {
      rep = "";
      line = list.at(line_ind);
      if (line == "coords") {
        OpenBabel::OBMol obmol = scene->OBMol();
        int natom = scene->numAtoms();
        int count = 1;
        FOR_ATOMS_OF_MOL(atom, obmol) {
          rep += static_cast<QString>(OpenBabel::etab.GetSymbol(atom->GetAtomicNum())) + " ";
          rep += QString::number(atom->GetX(), 'f', 6) + " ";
          rep += QString::number(atom->GetY(), 'f', 6) + " ";
          rep += QString::number(atom->GetZ(), 'f', 6);
          if (count != natom) rep += "\n";
          count++;
        }
      }
      if (line == "coordsId") {
        OpenBabel::OBMol obmol = scene->OBMol();
        int natom = scene->numAtoms();
        int count = 1;
        FOR_ATOMS_OF_MOL(atom, obmol) {
          rep += static_cast<QString>(OpenBabel::etab.GetSymbol(atom->GetAtomicNum())) + " ";
          rep += QString::number(atom->GetAtomicNum()) + " ";
          rep += QString::number(atom->GetX(), 'f', 6) + " ";
          rep += QString::number(atom->GetY(), 'f', 6) + " ";
          rep += QString::number(atom->GetZ(), 'f', 6);
          if (count != natom) rep += "\n";
          count++;
        }
      }
      else if (line == "numAtoms")	rep += QString::number(scene->numAtoms());
      else if (line == "filename") 	rep += scene->fileName();
      else if (line == "rempath") 	rep += scene->getRempath();
      else if (line == "id")	 	rep += QString::number(scene->getSceneNumber());

      // In case the replacement key is invalid, leave it
      else if (rep.isEmpty()) rep = line;

      // Actually replace the string
      list.replace(line_ind, rep);
    }

    // Rejoin string
    QString ret = list.join("");
    ret = ret.remove("%") + "\n";
    return ret;
  }

}
