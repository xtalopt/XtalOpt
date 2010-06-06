/**********************************************************************
  Xtal - Wrapper for Structure to ease work with crystals.

  Copyright (C) 2009-2010 by David C. Lonie

  This file is part of the Avogadro molecular editor project.
  For more information, see <http://avogadro.openmolecules.net/>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include <xtalopt/structures/xtal.h>

#include <avogadro/primitive.h>
#include <avogadro/molecule.h>
#include <avogadro/atom.h>

#include <openbabel/generic.h>
#include <openbabel/rand.h>
#include <openbabel/forcefield.h>

#include <QDebug>
#include <QRegExp>
#include <QStringList>

extern "C" {
#include <spglib/spglib.h>
}

using namespace std;
using namespace OpenBabel;
using namespace Eigen;

namespace Avogadro {

  Xtal::Xtal(QObject *parent) :
    Structure(parent)
  {
    ctor(parent);
  }

  Xtal::Xtal(double A, double B, double C,
             double Alpha, double Beta, double Gamma,
             QObject *parent) :
  Structure(parent)
  {
    ctor(parent);
    setCellInfo(A, B, C, Alpha, Beta, Gamma);
  }

  // Actual constructor:
  void Xtal::ctor(QObject *parent)
  {
    m_spgNumber = 231;
    this->setOBUnitCell(new OpenBabel::OBUnitCell);
    // Openbabel seems to be fond of making unfounded assumptions that
    // break things. This fixes one of them.
    this->cell()->SetSpaceGroup(0);
  }

  Xtal::~Xtal() {
  }

  void Xtal::setVolume(double  Volume) {

    // Get scaling factor
    double factor = pow(Volume/cell()->GetCellVolume(), 1.0/3.0); // Cube root

    // Store position of atoms in fractional units
    QList<Atom*> atomList       = atoms();
    QList<Vector3d*> fracCoordsList;
    for (int i = 0; i < atomList.size(); i++)
      fracCoordsList.append(cartToFrac(atomList.at(i)->pos()));

    // Scale cell
    setCellInfo(factor * cell()->GetA(),
                factor * cell()->GetB(),
                factor * cell()->GetC(),
                cell()->GetAlpha(),
                cell()->GetBeta(),
                cell()->GetGamma());

    // Recalculate coordinates:
    for (int i = 0; i < atomList.size(); i++)
      atomList.at(i)->setPos(fracToCart(*(fracCoordsList.at(i))));

    // Free memory
    qDeleteAll(fracCoordsList);
  }

  void Xtal::rescaleCell(double a, double b, double c, double alpha, double beta, double gamma) {

    if (!a && !b && !c && !alpha && !beta && !gamma) return;

    // Store position of atoms in fractional units
    QList<Atom*> atomList       = atoms();
    QList<Vector3d*> fracCoordsList;
    for (int i = 0; i < atomList.size(); i++)
      fracCoordsList.append(cartToFrac(atomList.at(i)->pos()));

    double nA = getA();
    double nB = getB();
    double nC = getC();
    double nAlpha = getAlpha();
    double nBeta = getBeta();
    double nGamma = getGamma();

    // Set angles and reset volume
    if (alpha || beta || gamma) {
      double volume = getVolume();
      if (alpha) nAlpha = alpha;
      if (beta) nBeta = beta;
      if (gamma) nGamma = gamma;
      setCellInfo(nA,
                  nB,
                  nC,
                  nAlpha,
                  nBeta,
                  nGamma);
      setVolume(volume);
    }

    if (a || b || c) {
      // Initialize variables
      double scale_primary, scale_secondary, scale_tertiary;

      // Set lengths while preserving volume
      // Case one length is static
      if (a && !b && !c) {        // A
        scale_primary = a / nA;
        scale_secondary = pow(scale_primary, 0.5);
        nA = a;
        nB *= scale_secondary;
        nC *= scale_secondary;
      }
      else if (!a && b && !c) {   // B
        scale_primary = b / nB;
        scale_secondary = pow(scale_primary, 0.5);
        nB = b;
        nA *= scale_secondary;
        nC *= scale_secondary;
      }
      else if (!a && !b && c) {   // C
        scale_primary = c / nC;
        scale_secondary = pow(scale_primary, 0.5);
        nC = c;
        nA *= scale_secondary;
        nB *= scale_secondary;
      }
      // Case two lengths are static
      else if (a && b && !c) {    // AB
        scale_primary     = a / nA;
        scale_secondary	= b / nB;
        scale_tertiary	= scale_primary * scale_secondary;
        nA = a;
        nB = b;
        nC *= scale_tertiary;
      }
      else if (a && !b && c) {    // AC
        scale_primary     = a / nA;
        scale_secondary	= c / nC;
        scale_tertiary	= scale_primary * scale_secondary;
        nA = a;
        nC = c;
        nB *= scale_tertiary;
      }
      else if (!a && b && c) {    // BC
        scale_primary     = c / nC;
        scale_secondary	= b / nB;
        scale_tertiary	= scale_primary * scale_secondary;
        nC = c;
        nB = b;
        nA *= scale_tertiary;
      }
      // Case three lengths are static
      else if (a && b && c) {     // ABC
        nA = a;
        nB = b;
        nC = c;
      }

      // Update unit cell
      setCellInfo(nA,
                  nB,
                  nC,
                  nAlpha,
                  nBeta,
                  nGamma);
    }

    // Recalculate coordinates:
    for (int i = 0; i < atomList.size(); i++)
      atomList.at(i)->setPos(fracToCart(fracCoordsList.at(i)));
  }

  bool Xtal::fixAngles(int attempts) {
    //qDebug() << "Xtal::fixAngles(" << attempts << ") called.";
    vector<vector3> vs = cell()->GetCellVectors();
    vector<vector3> vs_orig = cell()->GetCellVectors();
    //qDebug() << "V0" << vs_orig[0].x() << vs_orig[0].y() << vs_orig[0].z();
    //qDebug() << "V1" << vs_orig[1].x() << vs_orig[1].y() << vs_orig[1].z();
    //qDebug() << "V2" << vs_orig[2].x() << vs_orig[2].y() << vs_orig[2].z();

    int attempt=0, totalChanges=0, changes=0, limit=0;
    while (true) {
      attempt++;
      changes = 0;
      if (attempt > attempts) {
        qWarning() << "Xtal::fixAngles: Maximum attempts to fix angles reached. Bailing.";
        return false;
      }
      for (unsigned int i = 0; i < vs.size(); i++) {
        for (unsigned int j = 0; j < vs.size(); j++) {
          if (i != j) {
            limit = 0;
            while (limit < 20
                   &&
                   (vectorAngle(vs[i],vs[j]) < 60
                    ||
                    vectorAngle(vs[i],vs[j]) > 120)
                   &&
                   (vs[i].length() >= vs[j].length())
                   ) {
              //qDebug() << "Attempt " << limit << vectorAngle(vs[i], vs[j]);
              //qDebug() << "Vi " << vs[i].x() << vs[i].y() << vs[i].z();
              //qDebug() << "Vj " << vs[j].x() << vs[j].y() << vs[j].z();
              double nproj = abs(dot(vs[i],vs[j])/vs[j].length());
              int sign = (dot(vs[i],vs[j]) > 0) ? 1 : -1;
              vs[i] = vs[i] - ceil(nproj/vs[j].length()) * sign * vs[j];
              changes++;
              totalChanges++;
              limit++;
            }
            if (limit >= 20) {
              qWarning() << "Xtal::fixAngles: Maximum vector iteration reached. Bailing.";
              qWarning() << vectorAngle(vs[i],vs[j]);
              qWarning() << i << j;
              return false;
            }
          }
        }
      }
      if (changes == 0) break;
    }

    if (totalChanges == 0) {
      return true;
    }

    double newVolume = abs(dot(vs[0],cross(vs[1],vs[2])));
    if (getVolume() - newVolume > 1e-8){
      qWarning() << "Volume before ("
                 << getVolume()
                 << ") and after ("
                 << newVolume
                 << ") Xtal::fixAngles() doesn't match -- not updating cell.";
      return false;
    }
    setCellInfo(vs[0], vs[1], vs[2]);
    wrapAtomsToCell();
    findSpaceGroup();
    return true;
  }

  OpenBabel::OBUnitCell* Xtal::cell() const
  {
    return OBUnitCell();
  }


  bool Xtal::addAtomRandomly(uint atomicNumber, double minIAD, double maxIAD, double maxAttempts) {
    Q_UNUSED(maxIAD);
    // Random number generator
    OpenBabel::OBRandom rand (true);    // "true" uses system random numbers.
                                        // OB's alogrithm resulted in sheets of atoms sloped through the cell.
    rand.TimeSeed();
    double IAD = -1;
    int i = 0;
    vector3 cartCoords;

    // For first atom, add to 0, 0, 0
    if (numAtoms() == 0) {
      cartCoords = vector3 (0,0,0);
    }
    else {
      do {
        // Generate fractional coordinates
        IAD = -1;
        double x = rand.NextFloat();
        double y = rand.NextFloat();
        double z = rand.NextFloat();

        // Convert to cartesian coordinates and store
        vector3 fracCoords (x,y,z);
        cartCoords = fracToCart(fracCoords);
        if (minIAD != -1) {
          getNearestNeighborDistance(cartCoords[0],
                                     cartCoords[1],
                                     cartCoords[2],
                                     IAD);
        }
        else { break;};
        i++;
      } while (i < maxAttempts && IAD <= minIAD);

      if (i >= maxAttempts) return false;
    }

    Atom *atm = addAtom();
    Eigen::Vector3d pos (cartCoords[0],cartCoords[1],cartCoords[2]);
    atm->setPos(pos);
    atm->setAtomicNumber(static_cast<int>(atomicNumber));
    return true;
  }

  bool Xtal::getShortestInteratomicDistance(double & shortest) const {
    QList<Atom*> atomList = atoms();
    if (atomList.size() <= 1) return false; // Need at least two atoms!
    QList<Vector3d> atomPositions;
    for (int i = 0; i < atomList.size(); i++)
      atomPositions.push_back(*(atomList.at(i)->pos()));

    // Initialize vars
    //  Atomic Positions
    Vector3d v1= atomPositions.at(0);
    Vector3d v2= atomPositions.at(1);
    //  Unit Cell Vectors
    //  First get OB matrix, extract vectors, then convert to Eigen::Vector3d's
    matrix3x3 obcellMatrix = cell()->GetCellMatrix();
    vector3 obU1 = obcellMatrix.GetRow(0);
    vector3 obU2 = obcellMatrix.GetRow(1);
    vector3 obU3 = obcellMatrix.GetRow(2);
    Vector3d u1 (obU1.x(), obU1.y(), obU1.z());
    Vector3d u2 (obU2.x(), obU2.y(), obU2.z());
    Vector3d u3 (obU3.x(), obU3.y(), obU3.z());
    //  Find all combinations of unit cell vectors to get wrapped neighbors
    QList<Vector3d> uVecs;
    int s_1, s_2, s_3; // will be -1, 0, +1 multipliers
    for (s_1 = -1; s_1 <= 1; s_1++) {
      for (s_2 = -1; s_2 <= 1; s_2++) {
        for (s_3 = -1; s_3 <= 1; s_3++) {
          uVecs.append(s_1*u1 + s_2*u2 + s_3*u3);
        }
      }
    }

    shortest = abs((v1-v2).norm());
    double distance;

    // Find shortest distance
    for (int i = 0; i < atomList.size(); i++) {
      v1 = atomPositions.at(i);
      for (int j = i+1; j < atomList.size(); j++) {
        v2 = atomPositions.at(j);
        // Intercell
        distance = abs((v1-v2).norm());
        if (distance < shortest) shortest = distance;
        // Intracell
        for (int vecInd = 0; vecInd < uVecs.size(); vecInd++) {
          distance = abs(((v1+uVecs.at(vecInd))-v2).norm());
          if (distance < shortest) shortest = distance;
        }
      }
    }

    return true;
  }

  bool Xtal::getNearestNeighborDistance(double x, double y, double z, double & shortest) const {
    QList<Atom*> atomList = atoms();
    if (atomList.size() < 1) return false; // Need at least one atom!
    QList<Vector3d> atomPositions;
    for (int i = 0; i < atomList.size(); i++)
      atomPositions.push_back(*(atomList.at(i)->pos()));

    // Initialize vars
    //  Atomic Positions
    Vector3d v1 (x, y, z);
    Vector3d v2 = atomPositions.at(0);
    //  Unit Cell Vectors
    //  First get OB matrix, extract vectors, then convert to Eigen::Vector3d's
    matrix3x3 obcellMatrix = cell()->GetCellMatrix();
    vector3 obU1 = obcellMatrix.GetRow(0);
    vector3 obU2 = obcellMatrix.GetRow(1);
    vector3 obU3 = obcellMatrix.GetRow(2);
    Vector3d u1 (obU1.x(), obU1.y(), obU1.z());
    Vector3d u2 (obU2.x(), obU2.y(), obU2.z());
    Vector3d u3 (obU3.x(), obU3.y(), obU3.z());
    //  Find all combinations of unit cell vectors to get wrapped neighbors
    QList<Vector3d> uVecs;
    int s_1, s_2, s_3; // will be -1, 0, +1 multipliers
    for (s_1 = -1; s_1 <= 1; s_1++) {
      for (s_2 = -1; s_2 <= 1; s_2++) {
        for (s_3 = -1; s_3 <= 1; s_3++) {
          uVecs.append(s_1*u1 + s_2*u2 + s_3*u3);
        }
      }
    }

    shortest = abs((v1-v2).norm());
    double distance;

    // Find shortest distance
    for (int j = 0; j < atomList.size(); j++) {
      v2 = atomPositions.at(j);
      // Intercell
      distance = abs((v1-v2).norm());
      if (distance < shortest) shortest = distance;
      // Intracell
      for (int vecInd = 0; vecInd < uVecs.size(); vecInd++) {
        distance = abs(((v2+uVecs.at(vecInd))-v1).norm());
        if (distance < shortest) shortest = distance;
      }
    }
    return true;
  }

  bool Xtal::getNearestNeighborHistogram(QList<double> & distance, QList<double> & frequency, double min, double max, double step, Atom *atom) const {

    if (min > max && step > 0) {
      qWarning() << "Xtal::getNearestNeighborHistogram: min cannot be greater than max!";
      return false;
    }
    if (step < 0 || step == 0) {
      qWarning() << "Xtal::getNearestNeighborHistogram: invalid step size:" << step;
      return false;
    }

    // Populate distance list
    distance.clear();
    frequency.clear();
    double val = min;
    do {
      distance.append(val);
      frequency.append(0);
      val += step;
    } while (val < max);

    QList<Atom*> atomList = atoms();
    QList<Vector3d> atomPositions;
    for (int i = 0; i < atomList.size(); i++)
      atomPositions.push_back(*(atomList.at(i)->pos()));

    // Initialize vars
    //  Atomic Positions
    Vector3d v1= atomPositions.at(0);
    Vector3d v2= atomPositions.at(1);
    //  Unit Cell Vectors
    //  First get OB matrix, extract vectors, then convert to Eigen::Vector3d's
    matrix3x3 obcellMatrix = cell()->GetCellMatrix();
    vector3 obU1 = obcellMatrix.GetRow(0);
    vector3 obU2 = obcellMatrix.GetRow(1);
    vector3 obU3 = obcellMatrix.GetRow(2);
    Vector3d u1 (obU1.x(), obU1.y(), obU1.z());
    Vector3d u2 (obU2.x(), obU2.y(), obU2.z());
    Vector3d u3 (obU3.x(), obU3.y(), obU3.z());
    //  Find all combinations of unit cell vectors to get wrapped neighbors
    QList<Vector3d> uVecs;
    int s_1, s_2, s_3; // will be -1, 0, +1 multipliers
    for (s_1 = -1; s_1 <= 1; s_1++) {
      for (s_2 = -1; s_2 <= 1; s_2++) {
        for (s_3 = -1; s_3 <= 1; s_3++) {
          uVecs.append(s_1*u1 + s_2*u2 + s_3*u3);
        }
      }
    }
    double diff;

    // build histogram
    // Loop over all atoms
    if (atom == 0) {
      for (int i = 0; i < atomList.size(); i++) {
        v1 = atomPositions.at(i);
        for (int j = i+1; j < atomList.size(); j++) {
          v2 = atomPositions.at(j);
          // Intercell
          diff = abs((v1-v2).norm());
          for (int k = 0; k < distance.size(); k++) {
            double radius = distance.at(k);
            if (abs(diff-radius) < step/2) {
              frequency[k]++;
            }
          }
          // Intracell
          for (int vecInd = 0; vecInd < uVecs.size(); vecInd++) {
            diff = abs(((v1+uVecs.at(vecInd))-v2).norm());
            for (int k = 0; k < distance.size(); k++) {
              double radius = distance.at(k);
              if (abs(diff-radius) < step/2) {
                frequency[k]++;
              }
            }
          }
        }
      }
    }
    // Or, just the one requested
    else {
      v1 = *atom->pos();
      for (int j = 0; j < atomList.size(); j++) {
        v2 = atomPositions.at(j);
        // Intercell
        diff = abs((v1-v2).norm());
        for (int k = 0; k < distance.size(); k++) {
          double radius = distance.at(k);
          if (diff != 0 && abs(diff-radius) < step/2) {
            frequency[k]++;
          }
        }
        // Intracell
        for (int vecInd = 0; vecInd < uVecs.size(); vecInd++) {
          diff = abs(((v1+uVecs.at(vecInd))-v2).norm());
          for (int k = 0; k < distance.size(); k++) {
            double radius = distance.at(k);
            if (abs(diff-radius) < step/2) {
              frequency[k]++;
            }
          }
        }
      }
    }

    return true;
  }

  QList<Vector3d> Xtal::getAtomCoordsFrac() const {
    QList<Vector3d> list;
    QList<QString> symbols = getSymbols();
    QString symbol_ref;
    OpenBabel::OBMol obmol = OBMol();
    for (int i = 0; i < symbols.size(); i++) {
      symbol_ref = symbols.at(i);
      FOR_ATOMS_OF_MOL(atom,obmol) {
        QString symbol_cur = QString(OpenBabel::etab.GetSymbol(atom->GetAtomicNum()));
        if (symbol_cur == symbol_ref) {
          vector3 obvec = cartToFrac(atom->GetVector());
          list.append(Vector3d(obvec.x(), obvec.y(), obvec.z()));
        }
      }
    }
    return list;
  }

  Eigen::Vector3d Xtal::fracToCart(const Eigen::Vector3d & fracCoords) const {
    OpenBabel::vector3 obfrac (fracCoords.x(), fracCoords.y(), fracCoords.z());
    OpenBabel::vector3 obcart = cell()->FractionalToCartesian(obfrac);
    Eigen::Vector3d cartCoords (obcart.x(), obcart.y(), obcart.z());
    return cartCoords;
  }

  Eigen::Vector3d *Xtal::fracToCart(const Eigen::Vector3d *fracCoords) const {
    OpenBabel::vector3 obfrac (fracCoords->x(), fracCoords->y(), fracCoords->z());
    OpenBabel::vector3 obcart = cell()->FractionalToCartesian(obfrac);
    Eigen::Vector3d *cartCoords = new Eigen::Vector3d (obcart.x(), obcart.y(), obcart.z());
    return cartCoords;
  }

  Eigen::Vector3d Xtal::cartToFrac(const Eigen::Vector3d & cartCoords) const {
    OpenBabel::vector3 obcart (cartCoords.x(), cartCoords.y(), cartCoords.z());
    OpenBabel::vector3 obfrac = cell()->CartesianToFractional(obcart);
    Eigen::Vector3d fracCoords (obfrac.x(), obfrac.y(), obfrac.z());
    return fracCoords;
  }

  Eigen::Vector3d *Xtal::cartToFrac(const Eigen::Vector3d *cartCoords) const {
    OpenBabel::vector3 obcart (cartCoords->x(), cartCoords->y(), cartCoords->z());
    OpenBabel::vector3 obfrac = cell()->CartesianToFractional(obcart);
    Eigen::Vector3d *fracCoords = new Eigen::Vector3d (obfrac.x(), obfrac.y(), obfrac.z());
    return fracCoords;
  }

  void Xtal::wrapAtomsToCell() {
    //qDebug() << "Xtal::wrapAtomsToCell() called";
    // Store position of atoms in fractional units
    QList<Atom*> atomList       = atoms();
    QList<Vector3d> fracCoordsList;
    for (int i = 0; i < atomList.size(); i++)
      fracCoordsList.append(cartToFrac(*(atomList.at(i)->pos())));

    // wrap fractional coordinates to [0,1)
    for (int i = 0; i < fracCoordsList.size(); i++) {
      fracCoordsList[i][0] = fmod(fracCoordsList[i][0]+100, 1);
      fracCoordsList[i][1] = fmod(fracCoordsList[i][1]+100, 1);
      fracCoordsList[i][2] = fmod(fracCoordsList[i][2]+100, 1);
    }

    // Recalculate cartesian coordinates:
    Eigen::Vector3d cartCoord;
    for (int i = 0; i < atomList.size(); i++) {
      cartCoord = fracToCart(fracCoordsList.at(i));
      atomList.at(i)->setPos(cartCoord);
    }
  }

  void Xtal::save(QTextStream &out) {
    Structure::save(out);
  }

  void Xtal::load(QTextStream &in) {
    Structure::load(in);
  }

  QHash<QString, double> Xtal::getFingerprint() {
    QHash<QString, double> fp; // fingerprint hash
    fp.insert("enthalpy", getEnthalpy());
    fp.insert("volume", getVolume());
    fp.insert("spacegroup", getSpaceGroupNumber());
    return fp;
  }

  uint Xtal::getSpaceGroupNumber() {
    if (m_spgNumber > 230)
      findSpaceGroup();
    return m_spgNumber;
  }

  QString Xtal::getSpaceGroupSymbol() {
    if (m_spgNumber > 230)
      findSpaceGroup();
    return m_spgSymbol;
  }

  QString Xtal::getHTMLSpaceGroupSymbol() {
    if (m_spgNumber > 230)
      findSpaceGroup();

    QString s = m_spgSymbol;

    // Prepare HTML tags
    s.prepend("<HTML>");
    s.append("</HTML>");

    // "_X"  --> "<sub>X</sub>"
    int ind = s.indexOf("_");
    while (ind != -1) {
      s = s.insert(ind+2, "</sub>");
      s = s.replace(ind, 1, "<sub>");
      ind = s.indexOf("_");
    }

    // "-X"  --> "<span style="text-decoration: overline">X</span>"
    ind = s.indexOf("-");
    while (ind != -1) {
      s = s.insert(ind+2, "</span>");
      s = s.replace(ind, 1, "<span style=\"text-decoration: overline\">");
      ind = s.indexOf("-", ind+35);
    }

    return s;
  }

  void Xtal::findSpaceGroup(double prec) {
    // reset space group to 0 so we can exit if needed
    m_spgNumber = 0;
    m_spgSymbol = "Unknown";
    int num = numAtoms();

    // if no unit cell or atoms, exit
    if (!cell() || num == 0) {
      qWarning() << "Xtal::findSpaceGroup( " << prec << " ) called on atom with no cell or atoms!";
      return;
    }

    // get lattice matrix
    std::vector<OpenBabel::vector3> vecs = cell()->GetCellVectors();
    vector3 obU1 = vecs[0];
    vector3 obU2 = vecs[1];
    vector3 obU3 = vecs[2];
    double lattice[3][3] = {
      {obU1.x(), obU2.x(), obU3.x()},
      {obU1.y(), obU2.y(), obU3.y()},
      {obU1.z(), obU2.z(), obU3.z()}
    };

    // Get atom info
    double positions[num][3];
    int types[num];
    QList<Atom*> atomList = atoms();
    Eigen::Vector3d fracCoords;
    for (int i = 0; i < atomList.size(); i++) {
      fracCoords        = cartToFrac(*(atomList.at(i)->pos()));
      types[i]          = atomList.at(i)->atomicNumber();
      positions[i][0]   = fracCoords.x();
      positions[i][1]   = fracCoords.y();
      positions[i][2]   = fracCoords.z();
    }

    // Scale precision to the cell size
    double shortestLength = getA();
    shortestLength = (shortestLength < getB()) ? shortestLength : getB();
    shortestLength = (shortestLength < getC()) ? shortestLength : getC();
    if (shortestLength == 0) return;
    prec /= shortestLength;
    if (prec >=0.5) prec = 0.5;

    // qDebug() << "double lattice[3][3] = {";
    // for (int i = 0;  i < 3; i++) {
    //   qDebug() <<
    //     QString("  {%1, %2, %3},")
    //     .arg(QString::number(lattice[i][0]))
    //     .arg(QString::number(lattice[i][1]))
    //     .arg(QString::number(lattice[i][2]));
    // }
    // qDebug() << "  };\n";

    // qDebug() << "double position[][3] = {";
    // for (int i = 0;  i < num; i++) {
    //   qDebug() <<
    //     QString("  {%1, %2, %3},")
    //     .arg(QString::number(positions[i][0]))
    //     .arg(QString::number(positions[i][1]))
    //     .arg(QString::number(positions[i][2]));
    // }
    // qDebug() << "  };\n";

    // QString str; QTextStream t (&str);
    // t << "int types[] = {";
    // for (int i = 0;  i < num; i++) {
    //   t << types[i];
    //   if (i != num-1) t << ",";
    // }
    // t << "};";
    // qDebug() << str;

    // qDebug() << "int num_atom = " << num;
    // qDebug() << "double prec = " << prec;

    // Find primitive cell
    num = spg_find_primitive(lattice, positions, types, num, prec);

    //qDebug() << "Spglib spg_find_primitive returns: " << num;

    // if spglib cannot find the primitive cell, just set the number
    // of atoms to numAtoms, since this means the unit cell is the
    // primitive cell.
    if (num == 0) {
      //qDebug() << "Xtal::findSpaceGroup( " << prec << " ): spglib unable to find primitive cell. Using unit cell.";
      num = numAtoms();
    }

    // find spacegroup
    char symbol[21];
    m_spgNumber = spg_get_international(symbol, lattice, positions, types, num, prec);

    // Fail if m_spgNumber is still 0
    if (m_spgNumber == 0) {
      //qDebug() << "Xtal::findSpaceGroup( " << prec << " ): spglib unable to find spacegroup!";
      return;
    }

    // Set and clean up the symbol string
    m_spgSymbol = QString(symbol);
    m_spgSymbol.replace(QRegExp("\\s"), "");
    return;
  }

  void Xtal::getSpglibFormat() const {
    std::vector<OpenBabel::vector3> vecs = cell()->GetCellVectors();
    vector3 obU1 = vecs[0];
    vector3 obU2 = vecs[1];
    vector3 obU3 = vecs[2];

    QString t;
    QTextStream out (&t);

    out << "double lattice[3][3] = {\n"
        << "  {" << obU1.x() << ", " << obU2.x() << ", " << obU3.x() << "},\n"
        << "  {" << obU1.y() << ", " << obU2.y() << ", " << obU3.y() << "},\n"
        << "  {" << obU1.z() << ", " << obU2.z() << ", " << obU3.z() << "}};\n\n";

    out << "double position[][3] = {";
    for (unsigned int i = 0; i < numAtoms(); i++) {
      if (i == 0 || i != numAtoms()-1) out << ",";
      out << "\n";
      out << "  {" << atom(i)->pos()->x() << ", "
          << atom(i)->pos()->y() << ", "
          << atom(i)->pos()->z() << "}";
    }
    out << "};\n\n";
    out << "int types[] = { ";
    for (unsigned int i = 0; i < numAtoms(); i++) {
      if (i == 0 || i != numAtoms()-1) out << ", ";
      out << atom(i)->atomicNumber();
    }
    out << " };\n";
    out << "int num_atom = " << numAtoms() << ";\n";
    qDebug() << t;
  }

} // end namespace Avogadro

//#include "xtal.moc"
