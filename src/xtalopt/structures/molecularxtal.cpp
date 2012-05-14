/**********************************************************************
  MolecularXtal - Molecular-crystal themed subclass to Xtal

  Copyright (C) 2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include <xtalopt/structures/molecularxtal.h>

#include <xtalopt/structures/submolecule.h>
#include <xtalopt/molecularxtaloptimizer.h>

#include <globalsearch/macros.h>
#include <globalsearch/obeigenconv.h>

#include <avogadro/atom.h>
#include <avogadro/bond.h>

extern "C" {
#include <spglib/spglib.h>
}

#include <xtalcomp/xtalcomp.h>

using namespace Avogadro;

namespace XtalOpt {

  MolecularXtal::MolecularXtal(QObject *parent) :
    Xtal(parent),
    m_preOptStepCount(0),
    m_preOptStep(0),
    m_needsPreOpt(false),
    m_mxtalOpt(NULL)
  {
  }

  MolecularXtal::MolecularXtal(double A, double B, double C,
                               double Alpha, double Beta, double Gamma,
                               QObject *parent) :
    Xtal(A, B, C, Alpha, Beta, Gamma, parent),
    m_preOptStepCount(0),
    m_preOptStep(0),
    m_needsPreOpt(false),
    m_mxtalOpt(NULL)
  {
  }

  MolecularXtal::MolecularXtal(const MolecularXtal &other) :
    Xtal(other.parent()),
    m_preOptStepCount(0),
    m_preOptStep(0),
    m_needsPreOpt(false),
    m_mxtalOpt(NULL)
  {
    *this = other;
  }

  MolecularXtal::~MolecularXtal()
  {
    for (int i = 0; i < this->m_subMolecules.size(); ++i) {
      delete this->m_subMolecules.at(i);
      this->m_subMolecules[i] = NULL;
    }
    this->m_subMolecules.clear();
  }

  GlobalSearch::Structure& MolecularXtal::copyStructure(
      const GlobalSearch::Structure &other)
  {
    this->Xtal::copyStructure(other);

    const MolecularXtal *otherMXtal =
        qobject_cast<const MolecularXtal*>(&other);
    if (otherMXtal == NULL) {
      qWarning() << Q_FUNC_INFO << "Other structure is not a MolecularXtal.";
      return *this;
    }

    // Remove existing submolecules
    for (int i = 0; i < this->m_subMolecules.size(); ++i) {
      delete this->m_subMolecules[i];
      this->m_subMolecules[i] = NULL;
    }
    m_subMolecules.clear();
#if QT_VERSION > 0x040700
    m_subMolecules.reserve(otherMXtal->m_subMolecules.size());
#endif // QT > 4.7.0

    // Copy the submolecules from other to this
    for (QList<SubMolecule*>::const_iterator
         it = otherMXtal->m_subMolecules.constBegin(),
         it_end = otherMXtal->m_subMolecules.constEnd(); it != it_end; ++it) {

      SubMolecule *sub = new SubMolecule ();
      sub->m_sourceId = (*it)->m_sourceId;
      sub->m_mxtal = this;
#if QT_VERSION > 0x040700
      sub->m_atoms.reserve((*it)->m_atoms.size());
#endif // QT > 4.7.0

      for (QList<Atom*>::const_iterator ait = (*it)->m_atoms.constBegin(),
           ait_end = (*it)->m_atoms.constEnd(); ait != ait_end; ++ait) {
        sub->m_atoms.append(this->atomById((*ait)->id()));
      }

#if QT_VERSION > 0x040700
	  sub->m_bonds.reserve((*it)->m_bonds.size());
#endif // QT > 4.7.0
      for (QList<Bond*>::const_iterator bit = (*it)->m_bonds.constBegin(),
           bit_end = (*it)->m_bonds.constEnd(); bit != bit_end; ++bit) {
        sub->m_bonds.append(this->bondById((*bit)->id()));
      }

      this->m_subMolecules.append(sub);
    }

    return *this;
  }

  bool MolecularXtal::operator ==(const MolecularXtal &other) const
  {
    // Compare coordinates using the default tolerances
    if (!this->MolecularXtal::compareCoordinates(other))
      return false;

    return true;
  }

  bool MolecularXtal::compareCoordinates(const MolecularXtal &other,
                                         const double lengthTol,
                                         const double angleTol) const
  {
    // Cell matrices as row vectors
    const OpenBabel::matrix3x3 thisCellOB (this->cell()->GetCellMatrix());
    const OpenBabel::matrix3x3 otherCellOB (other.cell()->GetCellMatrix());
    XcMatrix thisCell (thisCellOB(0,0), thisCellOB(0,1), thisCellOB(0,2),
                       thisCellOB(1,0), thisCellOB(1,1), thisCellOB(1,2),
                       thisCellOB(2,0), thisCellOB(2,1), thisCellOB(2,2));
    XcMatrix otherCell(otherCellOB(0,0), otherCellOB(0,1), otherCellOB(0,2),
                       otherCellOB(1,0), otherCellOB(1,1), otherCellOB(1,2),
                       otherCellOB(2,0), otherCellOB(2,1), otherCellOB(2,2));

    // vectors of fractional coordinates and atomic numbers
    std::vector<XcVector> thisCoords;
    std::vector<XcVector> otherCoords;
    std::vector<unsigned int> thisTypes;
    std::vector<unsigned int> otherTypes;
    thisCoords.reserve(this->numAtoms());
    thisTypes.reserve(this->numAtoms());
    otherCoords.reserve(other.numAtoms());
    otherTypes.reserve(other.numAtoms());
    Eigen::Vector3d pos;
    for (QList<Atom*>::const_iterator it = this->m_atomList.constBegin(),
           it_end = this->m_atomList.constEnd(); it != it_end; ++it) {
      // Don't compare hydrogens -- too floppy.
      if ((*it)->isHydrogen()) {
        continue;
      }
      pos = this->cartToFrac(*(*it)->pos());
      thisCoords.push_back(XcVector(pos.x(), pos.y(), pos.z()));
      thisTypes.push_back((*it)->atomicNumber());
    }
    for (QList<Atom*>::const_iterator it = other.m_atomList.constBegin(),
           it_end = other.m_atomList.constEnd(); it != it_end; ++it) {
      // Don't compare hydrogens -- too floppy.
      if ((*it)->isHydrogen()) {
        continue;
      }
      pos = other.cartToFrac(*(*it)->pos());
      otherCoords.push_back(XcVector(pos.x(), pos.y(), pos.z()));
      otherTypes.push_back((*it)->atomicNumber());
    }

    return XtalComp::compare(thisCell,  thisTypes,  thisCoords,
                             otherCell, otherTypes, otherCoords,
                             NULL, lengthTol, angleTol);
  }

  int MolecularXtal::numSubMolecules() const
  {
    return m_subMolecules.size();
  }

  QList<SubMolecule*> MolecularXtal::subMolecules() const
  {
    return m_subMolecules;
  }

  SubMolecule * MolecularXtal::subMolecule(int index)
  {
    return this->m_subMolecules.at(index);
  }

  const SubMolecule * MolecularXtal::subMolecule(int index) const
  {
    return this->m_subMolecules.at(index);
  }

  bool MolecularXtal::verifySubMolecules() const
  {
    // Cache and pass the cell matrix to each submolecule
    const Eigen::Matrix3d cellRowMatrix =
        OB2Eigen(this->cell()->GetCellMatrix());

    // Verify bonds in submolecules
    for (QList<SubMolecule*>::const_iterator it = m_subMolecules.constBegin(),
         it_end = m_subMolecules.constEnd(); it != it_end; ++it) {
      if (!(*it)->verifyBonds(cellRowMatrix)) {
        return false;
      }
    }

    return true;
  }

  // helper function. Returns -1 if the atom is not "between" the bonded atoms
  inline double squaredDistanceToBond(const Eigen::Vector3d &bondVec,
                                      const Eigen::Vector3d &startAtomPos,
                                      const Eigen::Vector3d &testPos,
                                      const double bondLengthSquared)
  {
    const Eigen::Vector3d beginToPos (testPos - startAtomPos);
    const Eigen::Vector3d proj
        ((bondVec.dot(beginToPos) / bondVec.squaredNorm()) * bondVec);

    // If the dot product of the projection and the bond vector is negative,
    // then the test point is on the other side of startAtom than the bond.
    if (proj.dot(bondVec) < 0.0) {
      return -1.0;
    }

    // If the projection vector is larger than the bond, then the atom is on
    // the other side of the endAtom.
    if (proj.squaredNorm() > bondLengthSquared) {
      return -1.0;
    }

    return (proj - beginToPos).squaredNorm();
  }

  bool MolecularXtal::checkAtomToBondDistances(double minDistance)
  {
    const double r2 = minDistance * minDistance;

    // Cache list of translations
    Eigen::Vector3d translations[27];
    std::vector<OpenBabel::vector3> obvecs
        (this->OBUnitCell()->GetCellVectors());
    const Eigen::Vector3d v1 (obvecs[0].AsArray());
    const Eigen::Vector3d v2 (obvecs[1].AsArray());
    const Eigen::Vector3d v3 (obvecs[2].AsArray());

    int transIndex = -1;
    for (double x = -1.0; x < 1.5; ++x) {
      for (double y = -1.0; y < 1.5; ++y) {
        for (double z = -1.0; z < 1.5; ++z) {
          translations[++transIndex] = x * v1 + y * v2 + z * v3;
        }
      }
    }

    // For each submolecule:
    for (QList<SubMolecule*>::const_iterator it = m_subMolecules.constBegin(),
         it_end = m_subMolecules.constEnd(); it != it_end; ++it) {

      // Get the coherent coordinates
      const QVector<Eigen::Vector3d> cohVecs ((*it)->getCoherentCoordinates());
      Q_ASSERT(cohVecs.size() == (*it)->m_atoms.size());

      // For each bond in current submolecule
      for (QList<Bond*>::const_iterator bit = (*it)->m_bonds.constBegin(),
           bit_end = (*it)->m_bonds.constEnd(); bit != bit_end; ++bit) {

        // Cache the bondVector and the start atom position
        Atom *begin = (*bit)->beginAtom();
        Atom *end = (*bit)->endAtom();
        const Eigen::Vector3d &beginPos = cohVecs[(*it)->m_atoms.indexOf(begin)];
        const Eigen::Vector3d &endPos   = cohVecs[(*it)->m_atoms.indexOf(end)];
        const Eigen::Vector3d bondVec (endPos - beginPos);
        const double bondLengthSquared = bondVec.squaredNorm();

        // For each atom in mxtal
        for (QList<Atom*>::const_iterator ait = m_atomList.constBegin(),
             ait_end = m_atomList.constEnd(); ait != ait_end; ++ait) {

          // Skip the atoms in the current bond
          if ((*ait) == begin || (*ait) == end)
            continue;

          // Cache a reference to this atom's original position
          const Eigen::Vector3d &testPos (*(*ait)->pos());

          // Test each translated position.
          for (int i = 0; i < 27; ++i) {

            // Calculate the distance from the atom to the bond
            const double dist = squaredDistanceToBond(
                  bondVec, beginPos, testPos + translations[i],
                  bondLengthSquared);

            // If dist is less than 0, then the test atom is not between the
            // bonded atoms ( and thus ok )
            if (dist < 0.0)
              continue;

            // If the distance is too small, return false
            if (dist < r2) {
              return false;

            } // if (distance < r2)
          } // For each translation
        } // For each atom
      } // foreach bond
    } // foreach submolecule

    // If all atoms pass, return true
    return true;
  }

  void MolecularXtal::findSpaceGroup(double prec)
  {
    // Check that the precision is reasonable
    if (prec < 1e-5) {
      qWarning() << "MolecularXtal::findSpaceGroup called with a precision of"
                 << prec << ". This is likely an error. Resetting prec to"
                 << 0.05 << ".";
      prec = 0.05;
    }

    // reset space group to 0 so we can exit if needed
    m_spgNumber = 0;
    m_spgSymbol = "Unknown";
    int num = this->numAtoms();
    // Remove hydrogens from count
    foreach (const Atom *a, this->m_atomList) {
      if (a->isHydrogen())
        --num;
    }

    // if no unit cell or atoms, exit
    if (!cell() || num == 0) {
      qWarning() << "MolecularXtal::findSpaceGroup(" << prec << ") called on "
                    "atom with no cell or atoms!";
      return;
    }

    // get lattice matrix
    std::vector<OpenBabel::vector3> vecs = cell()->GetCellVectors();
    OpenBabel::vector3 obU1 = vecs[0];
    OpenBabel::vector3 obU2 = vecs[1];
    OpenBabel::vector3 obU3 = vecs[2];
    double lattice[3][3] = {
      {obU1.x(), obU2.x(), obU3.x()},
      {obU1.y(), obU2.y(), obU3.y()},
      {obU1.z(), obU2.z(), obU3.z()}
    };

    // Get atom info
    double (*positions)[3] = new double[num][3];
    int *types = new int[num];
    Eigen::Vector3d fracCoord;
    int ind = 0;
    foreach (const Atom *a, this->m_atomList) {
      if (a->isHydrogen())
        continue;
      types[ind] = a->atomicNumber();
      fracCoord = this->cartToFrac(*a->pos());
      positions[ind][0] = fracCoord.x();
      positions[ind][1] = fracCoord.y();
      positions[ind][2] = fracCoord.z();
      ++ind;
    }

    // find spacegroup
    char symbol[21];
    m_spgNumber = spg_get_international(symbol,
                                        lattice,
                                        positions,
                                        types,
                                        num, prec);

    delete [] positions;
    delete [] types;

    // Update the OBUnitCell object.
    cell()->SetSpaceGroup(m_spgNumber);

    // Fail if m_spgNumber is still 0
    if (m_spgNumber == 0) {
      return;
    }

    // Set and clean up the symbol string
    m_spgSymbol = QString(symbol);
    m_spgSymbol.remove(" ");
    return;
  }

  bool MolecularXtal::isPreoptimizing() const
  {
    return (m_mxtalOpt == NULL) ? false : m_mxtalOpt->isRunning();
  }

  void MolecularXtal::setVolume(double Volume)
  {
    // Get scaling factor
    double factor = pow(Volume/cell()->GetCellVolume(), 1.0/3.0); // Cube root

    // Store submolecule centers in fractional units
    QList<SubMolecule*> submols = this->subMolecules();
    QList<Eigen::Vector3d> fracCenterList;
    for (int i = 0; i < submols.size(); i++)
      fracCenterList.append(cartToFrac(submols.at(i)->center()));

    // Scale cell
    setCellInfo(factor * cell()->GetA(),
                factor * cell()->GetB(),
                factor * cell()->GetC(),
                cell()->GetAlpha(),
                cell()->GetBeta(),
                cell()->GetGamma());

    // Recalculate coordinates:
    for (int i = 0; i < submols.size(); i++)
      submols.at(i)->setCenter(fracToCart(fracCenterList.at(i)));
  }

  void MolecularXtal::rescaleCell(double a, double b, double c,
                                  double alpha, double beta, double gamma)
  {
    if (!a && !b && !c && !alpha && !beta && !gamma) {
      return;
    }

    this->rotateCellAndCoordsToStandardOrientation();

    // Store position of atoms in fractional units
    QList<SubMolecule*> submols = this->subMolecules();
    QList<Eigen::Vector3d> fracCoordsList;
    for (int i = 0; i < submols.size(); i++)
      fracCoordsList.append(cartToFrac(submols.at(i)->center()));

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
        scale_primary   = a / nA;
        scale_secondary = b / nB;
        scale_tertiary  = scale_primary * scale_secondary;
        nA = a;
        nB = b;
        nC *= scale_tertiary;
      }
      else if (a && !b && c) {    // AC
        scale_primary   = a / nA;
        scale_secondary = c / nC;
        scale_tertiary  = scale_primary * scale_secondary;
        nA = a;
        nC = c;
        nB *= scale_tertiary;
      }
      else if (!a && b && c) {    // BC
        scale_primary   = c / nC;
        scale_secondary = b / nB;
        scale_tertiary  = scale_primary * scale_secondary;
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
    for (int i = 0; i < submols.size(); i++)
      submols.at(i)->setCenter(fracToCart(fracCoordsList.at(i)));
  }

  void MolecularXtal::addSubMolecule(SubMolecule *sub)
  {
    // Is the handle/submolecule valid?
    if (!sub || sub->numAtoms() == 0) {
      qWarning() << Q_FUNC_INFO << "Attempt to add NULL/empty submolecule?";
      return;
    }

    // Do we already own this subMolecule?
    if (m_subMolecules.contains(sub)) {
      qWarning() << "Attempting to add SubMolecule @" << sub
                 << " twice. Aborting.";
      return;
    }

    // Create atoms and bonds for the subMolecule to use
    QList<Atom*> atoms;
    QList<Bond*> bonds;
#if QT_VERSION >= 0x040700
    atoms.reserve(sub->numAtoms());
    atoms.reserve(sub->numBonds());
#endif

    for (int i = 0; i < sub->numAtoms(); ++i) {
      atoms.append(this->addAtom());
    }
    for (int i = 0; i < sub->numBonds(); ++i) {
      bonds.append(this->addBond());
    }

    // Take ownership of the submolecule:
    sub->takeOwnership(this, atoms, bonds);

    m_subMolecules.append(sub);
  }

  void MolecularXtal::removeSubMolecule(SubMolecule *sub)
  {
    // Is the handle/submolecule valid?
    if (!sub) {
      qWarning() << Q_FUNC_INFO << "Attempt to remove NULL submolecule?";
      return;
    }

    if (!m_subMolecules.contains(sub)) {
      qWarning() << "Attempting to remove SubMolecule @" << sub
                 << ", which does not belong to this MolecularXtal ("
                 << this->getIDString() << " @" << this << ". Aborting.";
      return;
    }

    // Cache list of atoms and bonds owned by submolecule
    QList<Atom*> atoms = sub->atoms();
    QList<Bond*> bonds = sub->bonds();

    m_subMolecules.removeAll(sub);
    sub->releaseOwnership();

    // Remove atoms and bonds previously held by submolecule.
    // Take out bonds first.
    for (QList<Bond*>::const_iterator it = bonds.constBegin(),
         it_end = bonds.constEnd(); it != it_end; ++it) {
      this->removeBond(*it);
    }
    for (QList<Atom*>::const_iterator it = atoms.constBegin(),
         it_end = atoms.constEnd(); it != it_end; ++it) {
      this->removeAtom(*it);
    }

  }

  void MolecularXtal::replaceSubMolecule(int i, SubMolecule *newSub)
  {
    SubMolecule *oldSub = m_subMolecules.at(i);
    this->removeSubMolecule(oldSub);
    this->addSubMolecule(newSub);

    // Move the new submol to position i
    m_subMolecules.insert(i, m_subMolecules.takeLast());

    // Clean up the old submol
    oldSub->deleteLater();
  }

  void MolecularXtal::readMolecularXtalSettings(const QString &filename)
  {
    SETTINGS(filename);

    if (!settings->value("molecularxtal/mxtalIsValid", false).toBool()) {
      qWarning() << "Cannot read invalid mxtal settings for mxtal"
                 << this->getIDString();
      return;
    }

    // Clear existing bonds. These will be recreated below
    while (m_bondList.size() != 0) {
      this->removeBond(m_bondList.first());
    }

    int lengSubMoleculeArr =
        settings->beginReadArray("molecularxtal/submolecules");
    for (int i = 0; i < lengSubMoleculeArr; ++i) {
      settings->setArrayIndex(i);

      // Create empty submolecule data
      SubMolecule *sub = new SubMolecule(this);

      sub->setSourceId(static_cast<unsigned long>(
                         settings->value("sourceId", -1).toULongLong()));

      // Read atom indices
      int lenAtomArr = settings->beginReadArray("atoms");
      for (int j = 0; j < lenAtomArr; ++j) {
        settings->setArrayIndex(j);
        int ind = settings->value("ind", -1).toInt();
        if (ind < 0 || ind > this->numAtoms() - 1) {
          qFatal(QString("Invalid atom index %1 for Structure %2 in %3.")
                 .arg(ind).arg(this->getIDString()).arg(filename)
                 .toStdString().c_str());
        }
        sub->m_atoms.append(this->atom(ind));
      }
      settings->endArray();

      // Read bond info
      int lenBondArr = settings->beginReadArray("bonds");
      for (int j = 0; j < lenBondArr; ++j) {
        settings->setArrayIndex(j);
        int begInd = settings->value("begInd", -1).toInt();
        int endInd = settings->value("endInd", -1).toInt();
        short order = static_cast<short>(settings->value("order", -1).toInt());
        if (begInd < 0 || begInd > this->numAtoms() - 1) {
          qFatal(QString("Invalid atom index %1 for Structure %2 in %3.")
                 .arg(begInd).arg(this->getIDString()).arg(filename)
                 .toStdString().c_str());
        }
        if (endInd < 0 || endInd > this->numAtoms() - 1) {
          qFatal(QString("Invalid atom index %1 for Structure %2 in %3.")
                 .arg(endInd).arg(this->getIDString()).arg(filename)
                 .toStdString().c_str());
        }
        Bond *bond = this->addBond();
        bond->setBegin(m_atomList[begInd]);
        bond->setEnd(m_atomList[endInd]);
        bond->setOrder(order);
        sub->m_bonds.append(bond);
      }
      settings->endArray();

      // Add submolecule to this
      sub->m_mxtal = this;
      m_subMolecules.append(sub);

    }
    settings->endArray();
  }

  void MolecularXtal::writeMolecularXtalSettings(const QString &filename) const
  {
    SETTINGS(filename);

    if (this->m_optimizerLookup.isEmpty()) {
      settings->setValue("molecularxtal/mxtalIsValid", false);
      return; // Cannot serialize yet
    }
    settings->setValue("molecularxtal/mxtalIsValid", true);

    settings->beginWriteArray("molecularxtal/submolecules");
    for (int i = 0; i < this->numSubMolecules(); ++i) {
      settings->setArrayIndex(i);

      // cache submolecule data
      const SubMolecule *sub = this->subMolecule(i);
      settings->setValue("sourceId", static_cast<unsigned long long>(
                           sub->sourceId()));
      const QList<Atom*> atoms = sub->atoms();
      const QList<Bond*> bonds = sub->bonds();

      // Write atom optimizer indicies using m_optimizerLookup table
      settings->beginWriteArray("atoms");
      for (int j = 0; j < atoms.size(); ++j) {
        settings->setArrayIndex(j);
        settings->setValue("ind", static_cast<int>(atoms.at(j)->index()));
      }
      settings->endArray();

      // Write bond ids using m_optimizerLookup table
      settings->beginWriteArray("bonds");
      for (int j = 0; j < bonds.size(); ++j) {
        settings->setArrayIndex(j);
        Bond *curbond = bonds[j];
        int begIndex = static_cast<int>(curbond->beginAtom()->index());
        int endIndex = static_cast<int>(curbond->endAtom()->index());
        settings->setValue("begInd", begIndex);
        settings->setValue("endInd", endIndex);
        settings->setValue("order", static_cast<int>(curbond->order()));
      }
      settings->endArray();
    }
    settings->endArray();
  }

  void MolecularXtal::abortPreoptimization() const
  {
    if (m_mxtalOpt != NULL) {
      m_mxtalOpt->abort();
      m_mxtalOpt->waitForFinished();
    }
  }

  void MolecularXtal::makeCoherent()
  {
    foreach (SubMolecule *sub, m_subMolecules)
      sub->makeCoherent();
  }

} // end namespace XtalOpt
