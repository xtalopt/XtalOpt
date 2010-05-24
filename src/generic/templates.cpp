/**********************************************************************
  XtalOpt - Tools for advanced crystal optimization

  Copyright (C) 2009-2010 by David Lonie

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

#include "xtal.h"
#include "optbase.h"
#include "optimizer.h"

#include <QDebug>
#include <QFile>
#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>

#define ANGSTROM_TO_BOHR 1.889725989

using namespace OpenBabel;
using namespace std;

namespace Avogadro {

  void XtalOptTemplate::showHelp() {
    //qDebug() << "XtalOptTemplate::showHelp() called";

    QString str;
    QTextStream out (&str);
    out
      << "The following keywords should be used instead of the indicated variable data:\n\n"
      << "%userX% -- User specified value, where X = 1, 2, 3, or 4\n"
      << "%coords% -- cartesian coordinate data\n\t[symbol] [x] [y] [z]\n"
      << "%coordsId% -- cartesian coordinate data with atomic number\n\t[symbol] [atomic number] [x] [y] [z]\n"
      << "%coordsFrac% -- fractional coordinate data\n\t[symbol] [x] [y] [z]\n"
      << "%coordsFracId% -- fractional coordinate data with atomic number\n\t[symbol] [atomic number] [x] [y] [z]\n"
      << "%cellMatrixAngstrom% -- Cell matrix in Angstrom\n"
      << "%cellVector1Angstrom% -- First cell vector in Angstrom\n"
      << "%cellVector2Angstrom% -- Second cell vector in Angstrom\n"
      << "%cellVector3Angstrom% -- Third cell vector in Angstrom\n"
      << "%cellMatrixBohr% -- Cell matrix in Bohr\n"
      << "%cellVector1Bohr% -- First cell vector in Bohr\n"
      << "%cellVector2Bohr% -- Second cell vector in Bohr\n"
      << "%cellVector3Bohr% -- Third cell vector in Bohr\n"
      << "%description% -- Optimization description\n"
      << "%a% -- Lattice parameter A\n"
      << "%b% -- Lattice parameter B\n"
      << "%c% -- Lattice parameter C\n"
      << "%alphaRad% -- Lattice parameter Alpha in rad\n"
      << "%betaRad% -- Lattice parameter Beta in rad\n"
      << "%gammaRad% -- Lattice parameter Gamma in rad\n"
      << "%alphaDeg% -- Lattice parameter Alpha in degrees\n"
      << "%betaDeg% -- Lattice parameter Beta in degrees\n"
      << "%gammaDeg% -- Lattice parameter Gamma in degrees\n"
      << "%volume% -- Unit cell volume\n"
      << "%numAtoms% -- Number of atoms in unit cell\n"
      << "%numSpecies% -- Number of unique atomic species in unit cell\n"
      << "%filename% -- local output filename\n"
      << "%rempath% -- path to xtal's remote directory\n"
      << "%gen% -- xtal generation number\n"
      << "%id% -- xtal id number\n"
      << "%optStep% -- current optimization step\n\n"
      << "%POSCAR% -- VASP poscar generator\n";

    QMessageBox::information(NULL, "Template Help", str);
  }

  QString XtalOptTemplate::interpretTemplate(const QString & str, Structure* structure, OptBase *p) {
    QStringList list = str.split("%");
    QString line;
    QString rep;
    Xtal *xtal = qobject_cast<Xtal*>(structure);
    for (int line_ind = 0; line_ind < list.size(); line_ind++) {
      rep = "";
      line = list.at(line_ind);
      if (line == "coords") {
        OpenBabel::OBMol obmol = structure->OBMol();
        FOR_ATOMS_OF_MOL(atom, obmol) {
          rep += static_cast<QString>(OpenBabel::etab.GetSymbol(atom->GetAtomicNum())) + " ";
          rep += QString::number(atom->GetX()) + " ";
          rep += QString::number(atom->GetY()) + " ";
          rep += QString::number(atom->GetZ()) + "\n";
        }
      }
      if (line == "coordsId") {
        OpenBabel::OBMol obmol = structure->OBMol();
        FOR_ATOMS_OF_MOL(atom, obmol) {
          rep += static_cast<QString>(OpenBabel::etab.GetSymbol(atom->GetAtomicNum())) + " ";
          rep += QString::number(atom->GetAtomicNum()) + " ";
          rep += QString::number(atom->GetX()) + " ";
          rep += QString::number(atom->GetY()) + " ";
          rep += QString::number(atom->GetZ()) + "\n";
        }
      }
      if (line == "coordsFrac") {
        OpenBabel::OBMol obmol = xtal->OBMol();
        FOR_ATOMS_OF_MOL(atom, obmol) {
          vector3 coords = xtal->cartToFrac(atom->GetVector());
          rep += static_cast<QString>(OpenBabel::etab.GetSymbol(atom->GetAtomicNum())) + " ";
          rep += QString::number(coords.x()) + " ";
          rep += QString::number(coords.y()) + " ";
          rep += QString::number(coords.z()) + "\n";
        }
      }
      if (line == "coordsFracId") {
        OpenBabel::OBMol obmol = xtal->OBMol();
        FOR_ATOMS_OF_MOL(atom, obmol) {
          vector3 coords = xtal->cartToFrac(atom->GetVector());
          rep += static_cast<QString>(OpenBabel::etab.GetSymbol(atom->GetAtomicNum())) + " ";
          rep += QString::number(atom->GetAtomicNum()) + " ";
          rep += QString::number(coords.x()) + " ";
          rep += QString::number(coords.y()) + " ";
          rep += QString::number(coords.z()) + "\n";
        }
      }
      if (line == "cellMatrixAngstrom") {
        matrix3x3 m = xtal->OBUnitCell()->GetCellMatrix();
        for (int i = 0; i < 3; i++) {
          for (int j = 0; j < 3; j++) {
            rep += QString::number(m.Get(i,j)) + "\t";
          }
          rep += "\n";
        }
      }
      if (line == "cellVector1Angstrom") {
        vector3 v = xtal->OBUnitCell()->GetCellVectors()[0];
        for (int i = 0; i < 3; i++) {
          rep += QString::number(v[i]) + "\t";
        }
      }
      if (line == "cellVector2Angstrom") {
        vector3 v = xtal->OBUnitCell()->GetCellVectors()[1];
        for (int i = 0; i < 3; i++) {
          rep += QString::number(v[i]) + "\t";
        }
      }
      if (line == "cellVector3Angstrom") {
        vector3 v = xtal->OBUnitCell()->GetCellVectors()[2];
        for (int i = 0; i < 3; i++) {
          rep += QString::number(v[i]) + "\t";
        }
      }
      if (line == "cellMatrixBohr") {
        matrix3x3 m = xtal->OBUnitCell()->GetCellMatrix();
        for (int i = 0; i < 3; i++) {
          for (int j = 0; j < 3; j++) {
            rep += QString::number(m.Get(i,j) * ANGSTROM_TO_BOHR) + "\t";
          }
          rep += "\n";
        }
      }
      if (line == "cellVector1Bohr") {
        vector3 v = xtal->OBUnitCell()->GetCellVectors()[0];
        for (int i = 0; i < 3; i++) {
          rep += QString::number(v[i] * ANGSTROM_TO_BOHR) + "\t";
        }
      }
      if (line == "cellVector2Bohr") {
        vector3 v = xtal->OBUnitCell()->GetCellVectors()[1];
        for (int i = 0; i < 3; i++) {
          rep += QString::number(v[i] * ANGSTROM_TO_BOHR) + "\t";
        }
      }
      if (line == "cellVector3Bohr") {
        vector3 v = xtal->OBUnitCell()->GetCellVectors()[2];
        for (int i = 0; i < 3; i++) {
          rep += QString::number(v[i] * ANGSTROM_TO_BOHR) + "\t";
        }
      }
      if (line == "user1")    		rep += p->optimizer()->getUser1();
      if (line == "user2")    		rep += p->optimizer()->getUser2();
      if (line == "user3")    		rep += p->optimizer()->getUser3();
      if (line == "user4")    		rep += p->optimizer()->getUser4();
      if (line == "a")			rep += QString::number(xtal->getA());
      else if (line == "b")             rep += QString::number(xtal->getB());
      else if (line == "c")             rep += QString::number(xtal->getC());
      else if (line == "alphaRad")	rep += QString::number(xtal->getAlpha() * DEG_TO_RAD);
      else if (line == "betaRad")       rep += QString::number(xtal->getBeta() * DEG_TO_RAD);
      else if (line == "gammaRad")      rep += QString::number(xtal->getGamma() * DEG_TO_RAD);
      else if (line == "alphaDeg")	rep += QString::number(xtal->getAlpha());
      else if (line == "betaDeg")       rep += QString::number(xtal->getBeta());
      else if (line == "gammaDeg")      rep += QString::number(xtal->getGamma());
      else if (line == "volume")        rep += QString::number(xtal->getVolume());
      else if (line == "numAtoms")	rep += QString::number(structure->numAtoms());
      else if (line == "numSpecies")	rep += QString::number(structure->getSymbols().size());
      else if (line == "description")	rep += p->description;
      else if (line == "filename")      rep += structure->fileName();
      else if (line == "rempath")       rep += structure->getRempath();
      else if (line == "gen")           rep += QString::number(structure->getGeneration());
      else if (line == "id")            rep += QString::number(structure->getIDNumber());
      else if (line == "incar")         rep += QString::number(structure->getCurrentOptStep());
      else if (line == "optStep")       rep += QString::number(structure->getCurrentOptStep());
      else if (line == "POSCAR") {
        // Comment line -- set to filename
        rep += xtal->fileName();
        rep += "\n";

        // Scaling factor. Just 1.0
        rep += QString::number(1.0);
        rep += "\n";

        // Unit Cell Vectors
        std::vector< vector3 > vecs = xtal->OBUnitCell()->GetCellVectors();
        for (uint i = 0; i < vecs.size(); i++) {
          rep += QString::number(vecs.at(i).x()) + " ";
          rep += QString::number(vecs.at(i).y()) + " ";
          rep += QString::number(vecs.at(i).z()) + " ";
          rep += "\n";
        }

        // Number of each type of atom (sorted alphabetically by symbol)
        QList<uint> list = xtal->getNumberOfAtomsAlpha();
        for (int i = 0; i < list.size(); i++)
          rep += QString::number(list.at(i)) + " ";
        rep += "\n";

        // Use fractional coordinates:
        rep += "Direct\n";

        // Coordinates of each atom (sorted alphabetically by symbol)
        QList<Eigen::Vector3d> coords = xtal->getAtomCoordsFrac();
        for (int i = 0; i < coords.size(); i++) {
          rep += QString::number(coords.at(i).x()) + " ";
          rep += QString::number(coords.at(i).y()) + " ";
          rep += QString::number(coords.at(i).z()) + " ";
          rep += "\n";
        }
      } // End %POSCAR%

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

  QString XtalOptTemplate::input_dump() {
    //qDebug() << "XtalOptTemplate::input_dump() called";
    QString ret;
    QTextStream out (&ret);
    out << "user1: %user1%\n"
        << "user2: %user2%\n"
        << "user3: %user3%\n"
        << "user4: %user4%\n"
        << "coords:\n%coords%\n\n"
        << "coordsId:\n%coordsId%\n\n"
        << "coordsFrac:\n%coordsFrac%\n\n"
        << "coordsFracId:\n%coordsFracId%\n\n"
        << "cellMatrixAngstrom:\n%cellMatrixAngstrom%\n\n"
        << "cellMatrixBohr:\n%cellMatrixBohr%\n\n"
        << "a: %a% -- Lattice parameter A\n"
        << "b: %b% -- Lattice parameter B\n"
        << "c: %c% -- Lattice parameter C\n"
        << "alphaRad: %alphaRad% -- Lattice parameter Alpha in rad\n"
        << "betaRad: %betaRad% -- Lattice parameter Beta in rad\n"
        << "gammaRad: %gammaRad% -- Lattice parameter Gamma in rad\n"
        << "alphaDeg: %alphaDeg% -- Lattice parameter Alpha in deg\n"
        << "betaDeg: %betaDeg% -- Lattice parameter Beta in deg\n"
        << "gammaDeg: %gammaDeg% -- Lattice parameter Gamma in deg\n"
        << "volume: %volume% -- Unit cell volume\n"
        << "description: %description% -- description\n"
        << "numAtoms: %numAtoms% -- Number of atoms in unit cell\n"
        << "numSpecies: %numSpecies% -- Number of unique atomic species in unit cell\n"
        << "filename: %filename% -- Name of output file\n\n"
        << "rempath: %rempath% -- path to xtal's remote directory\n"
        << "gen: %gen% -- xtal generation number\n"
        << "id: %id% -- xtal id number\n"
        << "optstep: %optstep% -- current optimization step for xtal\n\n"
        << "---------POSCAR------------\n"
        << "%POSCAR%\n"
        << "---------------------------\n";
    return ret;
  }

  QString XtalOptTemplate::input_VASP_POSCAR() {
    //qDebug() << "XtalOptTemplate::input_VASP_POSCAR() called";
    return "%POSCAR%";
  }

  QString XtalOptTemplate::input_VASP_KPOINTS() {
    //qDebug() << "XtalOptTemplate::input_VASP_KPOINTS() called";
    QString ret;
    QTextStream out (&ret);
    out << "Automatic generation\n"
        << "0\n"
        << "Auto\n"
        << "50\n";
    return ret;
  }

  QString XtalOptTemplate::input_VASP_INCAR() {
    //qDebug() << "XtalOptTemplate::input_VASP_INCAR() called";
    QString ret;
    QTextStream out (&ret);
    out << "%filename%\n\n"
        << "# output options\n"
        << "LWAVE  = .FALSE. # write or don't write WAVECAR\n"
        << "LCHARG = .FALSE. # write or don't write CHG and CHGCAR\n"
        << "LELF   = .FALSE. # write ELF\n\n"
        << "# ionic relaxation\n"
        << "NSW = 100        # number of ionic steps\n"
        << "IBRION = 2       # 2=conjucate gradient, 1=Newton like\n"
        << "ISIF = 3         # 3=relax everything, 2=relax ions only, 4=keep volume fixed\n\n"
        << "# precision parameters\n"
        << "EDIFF = 1E-3     # 1E-3 very low precision for pre-relaxation, use 1E-5 next\n"
        << "EDIFFG = 1E-2    # usually: 10 * EDIFF\n"
        << "PREC = med       # precision low, med, high, accurate\n\n"
        << "# electronic relaxation\n"
        << "ISMEAR = -5      # -5 = tetraedon, 1..N = Methfessel\n"
        << "ENCUT = 500      # cutoff energy\n";
    return ret;
  }
}
