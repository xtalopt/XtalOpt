/**********************************************************************
  SubMolecule - Manage molecular subunits of MolecularXtal

  Copyright (C) 2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include <xtalopt/structures/submolecule.h>
#include <xtalopt/structures/molecularxtal.h>

#include <globalsearch/macros.h>
#include <globalsearch/obeigenconv.h>

#include <avogadro/atom.h>
#include <avogadro/bond.h>
#include <avogadro/molecule.h>

#include <openbabel/generic.h>
#include <openbabel/mol.h> // for etab

#include <QtCore/QVector>

#include <Eigen/Core>
#include <Eigen/LeastSquares>
#include <Eigen/LU>
#include <Eigen/Geometry>

#include <limits>
#include <float.h>

using namespace Avogadro;

namespace XtalOpt {

  SubMolecule::SubMolecule(QObject *parent)
    : Primitive(parent),
      m_sourceId(std::numeric_limits<unsigned long>::max()),
      m_dummyParent(new MolecularXtal (this)),
      m_mxtal(NULL)
  {
  }

  SubMolecule::~SubMolecule()
  {
  }

  double SubMolecule::radius() const
  {
    const Eigen::Vector3d center (this->center());
    const Atom * farthest = this->farthestAtom();

    return (center - (*farthest->pos())).norm();
  }

  Avogadro::Atom * SubMolecule::farthestAtom() const
  {
    const Eigen::Vector3d centerPos (this->center());

    // Iterate through all atoms to locate the farthest
    const Atom *farthest = NULL;
    double shortest = DBL_MAX;
    // Just iterate through atoms if no unit cell available
    if (m_mxtal == NULL) {
      for (QList<Atom*>::const_iterator it = m_atoms.constBegin(),
           it_end = m_atoms.constEnd(); it != it_end; ++it) {
        const double current = (centerPos - (*(*it)->pos())).squaredNorm();
        if (current < shortest) {
          shortest = current;
          farthest = *it;
        }
      }
    }
    // Otherwise, we can shorten the diff vector using translational symm
    else {
      // The coherent coordinates are the same used when calculating the
      // center, so this is safe.
      Eigen::Vector3d diff;
      for (QList<Atom*>::const_iterator it = m_atoms.constBegin(),
           it_end = m_atoms.constEnd(); it != it_end; ++it) {
        diff = (*(*it)->pos()) - centerPos;
        m_mxtal->shortenCartesianVector(&diff);
        const double current = diff.squaredNorm();
        if (current < shortest) {
          shortest = current;
          farthest = *it;
        }
      }
    }
    Q_ASSERT_X(farthest != NULL, Q_FUNC_INFO, "Farthest atom not found. "
               "An atom position is likely NaN.");
    return const_cast<Atom*>(farthest);
  }

  const Eigen::Vector3d * SubMolecule::farthestAtomPos() const
  {
    return this->farthestAtom()->pos();
  }

  Eigen::Vector3d SubMolecule::center() const
  {
    Eigen::Vector3d sum (0.0, 0.0, 0.0);

    if (m_mxtal == NULL) { // Just use the atomic positions
      for (QList<Atom*>::const_iterator it = m_atoms.constBegin(),
           it_end = m_atoms.constEnd(); it != it_end; ++it) {
        sum += *(*it)->pos();
      }
      sum /= static_cast<double>(m_atoms.size());
    }
    else { // Otherwise, grab the coherent coordinates
      QVector<Eigen::Vector3d> cohCoords = this->getCoherentCoordinates();
      for (QVector<Eigen::Vector3d>::const_iterator it = cohCoords.constBegin(),
           it_end = cohCoords.constEnd(); it != it_end; ++it) {
        sum += *it;
      }
      sum /= static_cast<double>(cohCoords.size());
    }

    return sum;
  }

  Eigen::Vector3d SubMolecule::normal() const
  {
    // Taken from Avogadro::Molecule::computeGeomInfo:
    if (this->numAtoms() > 1) {
      // Compute the normal vector to the submolecule's best-fitting plane
      QVector<Eigen::Vector3d> vectorStore;
      Eigen::Vector3d ** atomPositions = new Eigen::Vector3d*[this->numAtoms()];
      // Just use atom positions if no associated mxtal
      if (m_mxtal == NULL) {
        int i = -1;
        vectorStore.reserve(this->numAtoms());
        foreach (Atom *atom, m_atoms) {
          vectorStore.append(*atom->pos());
          atomPositions[++i] = &vectorStore[i];
        }
      }
      // Otherwise, use coherent coordinates
      else {
        vectorStore = this->getCoherentCoordinates();
        for (int i = 0; i < vectorStore.size(); ++i) {
          atomPositions[i] = &vectorStore[i];
        }
      }
      Eigen::Hyperplane<double, 3> planeCoeffs;
      Eigen::fitHyperplane(this->numAtoms(), atomPositions, &planeCoeffs);
      delete [] atomPositions;

      // Replace with unit Z if the normal is invalid
      Eigen::Vector3d n (planeCoeffs.normal());
      double sqNorm = n.squaredNorm();
      if (sqNorm < 1e-5 || GS_IS_NAN_OR_INF(sqNorm)) {
        qDebug() << QString("Replacing normal with unit z. Mxtal %1, submol "
                            "%2,\n\tn.x = %3 n.y = %4 n.z = %5")
                    .arg((m_mxtal == NULL) ? "NULL" : m_mxtal->getIDString())
                    .arg((m_mxtal == NULL) ?
                           "N/A" : QString(m_mxtal->subMolecules().indexOf(
                                             const_cast<SubMolecule*>(this))))
                    .arg(n.x()).arg(n.y()).arg(n.z());
        n = Eigen::Vector3d(0, 0, 1);
      }

      return n;
    }
    else {
      return Eigen::Vector3d(0, 0, 1);
    }
  }

  QVector<Eigen::Vector3d> SubMolecule::getCoherentCoordinates(
      Avogadro::Atom *req_start, const Eigen::Matrix3d &req_cellRowMatrix) const
  {
    QVector<Eigen::Vector3d> retVec;
    if (req_start != NULL) {
      Q_ASSERT_X(m_atoms.contains(req_start), Q_FUNC_INFO, "Submolecule does "
                 "not contain starting atom!");
      qWarning() << "Submolecule does not contain starting atom!";
      return retVec;
    }

    if (m_atoms.size() == 0) {
      // No atoms!
      qWarning() << Q_FUNC_INFO << "called on a submolecule with no atoms!";
      return retVec;
    }

    // Use m_mxtals's cell matrix if none given
    Eigen::Matrix3d cellRowMatrix (req_cellRowMatrix);
    if (cellRowMatrix.isZero(1e-9)) {
      if (m_mxtal != NULL) {
        if (OpenBabel::OBUnitCell *cell = m_mxtal->OBUnitCell()) {
          cellRowMatrix = OB2Eigen(cell->GetCellMatrix());
        }
        else {
          qWarning() << Q_FUNC_INFO << "called with no translational symmetry "
                      "info! (no unit cell)";
          return retVec;
        }
      }
      else {
        qWarning() << Q_FUNC_INFO << "called with no translational symmetry "
                    "info! (isolated submolecule)";
        return retVec;
      }
    }

    // starting atom -- choose the atom that is closest to the origin
    Avogadro::Atom *start = req_start;
    if (start == NULL) {
      double shortestDistanceSquared = DBL_MAX;
      double currentDistanceSquared;
      for (QList<Atom*>::const_iterator it = m_atoms.constBegin(),
           it_end = m_atoms.constEnd(); it != it_end; ++it) {
        currentDistanceSquared = (*it)->pos()->squaredNorm();
        if (currentDistanceSquared < shortestDistanceSquared) {
          shortestDistanceSquared = currentDistanceSquared;
          start = (*it);
        }
      }
    }
    Q_ASSERT_X(start != NULL, Q_FUNC_INFO,"Starting atom could not be found");

    // Create cartesian and fractionalization matrices
    const Eigen::Matrix3d cart (cellRowMatrix.transpose());

    // Here we go! perform a breadth-first search for all of the atoms in
    // the submolecule, starting with "start". Iterate through each bond
    // on each atom, minimizing its length.
    QVector<Atom*> atomsFound;
    atomsFound.reserve(m_atoms.size());
    atomsFound.append(start);
    // Fill retVec with the initial positions of the atoms
    retVec.reserve(m_atoms.size());
    for (QList<Atom*>::const_iterator it = m_atoms.constBegin(),
         it_end = m_atoms.constEnd(); it != it_end; ++it) {
      retVec.push_back(*(*it)->pos());
    }

    // mxtal is used for atom/bond id lookups
    MolecularXtal *mxtal = (m_mxtal != NULL)
        ? m_mxtal
        : qobject_cast<MolecularXtal*>(start->parent());
    Q_ASSERT_X(mxtal != NULL, Q_FUNC_INFO,
               "Atoms in this submolecule are orphans!");

    QList<unsigned long> bondIds;
    Eigen::Vector3d bondVector;
    // Iterate through all atoms that have been found and find their coherent
    // coordinates
    for (int i = 0; i < atomsFound.size(); ++i) {
      Atom *curAtom = atomsFound[i];
      const int curIndex = m_atoms.indexOf(curAtom);
      bondIds = curAtom->bonds();
      // Iterate through all bonds in the current atom
      for (QList<unsigned long>::const_iterator it = bondIds.constBegin(),
           it_end = bondIds.constEnd(); it != it_end; ++it) {
        Bond *bond = mxtal->bondById(*it);
        // Find the other atom in this bond
        Atom *bondedAtom = (bond->beginAtom() == curAtom) ? bond->endAtom()
                                                          : bond->beginAtom();

        // Skip this atom if we've already found it.
        if (atomsFound.contains(bondedAtom)) {
          continue;
        }

        // Determine the index of the bonded atom
        const int bondedIndex = m_atoms.indexOf(bondedAtom);
        Q_ASSERT_X(bondedIndex != -1, Q_FUNC_INFO,
                   "An atom is bonded to an atom that does not belong to this"
                   " submolecule.");

        // Mark bonded atom as found
        atomsFound.append(bondedAtom);
        bondVector = retVec[bondedIndex] -
            retVec[curIndex];

        // Shorten the bond vector
        MolecularXtal::shortenCartesianVector(&bondVector, cart);

        // Store new position
        retVec[bondedIndex] = retVec[curIndex] + bondVector;
      }
    }

    if (atomsFound.size() != m_atoms.size()) {
      qWarning() << "Not all atoms found while making submolecule coherent. "
                  "Found" << atomsFound.size() << "of" << m_atoms.size();
      retVec.clear();
      return retVec;
    }

    return retVec;
  }

  void SubMolecule::makeCoherent(Avogadro::Atom *start,
                                 const Eigen::Matrix3d &cellRowMatrix) const
  {
    QVector<Eigen::Vector3d> cohCoords =
        this->getCoherentCoordinates(start, cellRowMatrix);
    if (cohCoords.isEmpty()) {
      qWarning() << Q_FUNC_INFO << "Coherent coordinate vector is empty.";
      return;
    }
    Q_ASSERT(cohCoords.size() == m_atoms.size());
    for (int i = 0; i < cohCoords.size(); ++i) {
      Atom *atom = m_atoms.at(i);
      atom->setPos(cohCoords[i]);
    }
  }

  SubMolecule * SubMolecule::getCoherentClone(Atom *start) const
  {
    if (start != NULL) {
      Q_ASSERT_X(m_atoms.contains(start), Q_FUNC_INFO, "Submolecule does not"
                 " contain starting atom!");
      qWarning() << "Submolecule does not contain starting atom!";
      return NULL;
    }

    SubMolecule *sub = new SubMolecule();

#if QT_VERSION >= 0x040700
    // Reserve space
    sub->m_atoms.reserve(numAtoms());
    sub->m_bonds.reserve(numBonds());
#endif

    // Create atoms
    for (int i = 0; i < this->numAtoms(); ++i) {
      sub->m_atoms.append(sub->m_dummyParent->addAtom());
    }

    // Create bonds
    for (int i = 0; i < this->numBonds(); ++i) {
      sub->m_bonds.append(sub->m_dummyParent->addBond());
    }

    // Copy atomic information over
    // Lookup table: key = old id, val = new index.
    QHash<unsigned long, unsigned long> atomIdLUT;
    for (int i = 0; i < sub->m_atoms.size(); ++i) {
      Atom *oldAtom = m_atoms.at(i);
      Atom *newAtom = sub->m_atoms.at(i);

      *newAtom = *oldAtom;

      atomIdLUT.insert(oldAtom->id(), i);
    }

    // Copy bond info
    for (int i = 0; i < sub->m_bonds.size(); ++i) {
      Bond *oldBond = m_bonds.at(i);
      Bond *newBond = sub->m_bonds.at(i);

      *newBond = *oldBond;

      newBond->setBegin(sub->m_atoms[atomIdLUT.value(oldBond->beginAtomId())]);
      newBond->setEnd(sub->m_atoms[atomIdLUT.value(oldBond->endAtomId())]);
    }

    // Get translation properties for makeCoherent
    if (m_mxtal != NULL) {
      if (OpenBabel::OBUnitCell *cell = m_mxtal->OBUnitCell()) {
        Eigen::Matrix3d cellRowMatrix = OB2Eigen(cell->GetCellMatrix());
        sub->makeCoherent(start, cellRowMatrix);
      }
      else {
        qWarning() << Q_FUNC_INFO << "Cannot make coherent -- no cell set.";
      }
    }
    else {
      qWarning() << Q_FUNC_INFO << "Cannot make coherent -- isolated submolecule.";
    }

    // Metadata
    sub->m_sourceId = this->m_sourceId;
    return sub;
  }

  bool SubMolecule::verifyBonds(const Eigen::Matrix3d &req_cellRowMatrix) const
  {
    bool hasTranslations = false;
    Eigen::Matrix3d cellRowMatrix;
    // Check is the requested translation matrix is valid
    if (!req_cellRowMatrix.isZero(1e-5)) {
      cellRowMatrix = req_cellRowMatrix;
      hasTranslations = true;
    }
    // Otherwise, check for a parent
    if (!hasTranslations && m_mxtal != NULL) {
      cellRowMatrix = OB2Eigen(m_mxtal->OBUnitCell()->GetCellMatrix());
      hasTranslations = true;
    }

    // Setup cartesian and fractionalization matrix
    Eigen::Matrix3d cart;
    if (hasTranslations) {
      cart = cellRowMatrix.transpose();
    }

    // Iterate through all bonds, checking end - begin distances, including
    // translational symmetry if needed
    Eigen::Vector3d bondVec;
    for (QList<Bond*>::const_iterator it = m_bonds.constBegin(),
         it_end = m_bonds.constEnd(); it != it_end; ++it) {
      const Eigen::Vector3d *begPos ((*it)->beginPos());
      const Eigen::Vector3d *endPos ((*it)->endPos());
      bondVec = *endPos - *begPos;
      const double maxDist = (
            OpenBabel::etab.GetCovalentRad((*it)->beginAtom()->atomicNumber()) +
            OpenBabel::etab.GetCovalentRad((*it)->endAtom()->atomicNumber()))
          * 2.0; // fudge factor
      const double maxDistSquared = maxDist * maxDist;

      // Shorten bond vec if needed
      if (hasTranslations) {
        MolecularXtal::shortenCartesianVector(&bondVec, cart);
      }

      if (bondVec.squaredNorm() > maxDistSquared) {
        return false;
      }
    }

    return true;
  }

  void SubMolecule::takeOwnership(MolecularXtal *parent,
                                  const QList<Atom*> &atoms,
                                  const QList<Bond*> &bonds)
  {
    // Is the new parent valid? If not, releaseOwnership instead
    if (parent == NULL) {
      qWarning() << Q_FUNC_INFO << "A NULL MolecularXtal cannot take ownership "
                  "of a submolecule. Releasing ownership instead.";
      if (atoms.size() || bonds.size()) {
        qWarning() << "Atom and bond lists will be ignored.";
      }
      this->releaseOwnership();
      return;
    }

    // Is there an existing m_mxtal? If so, releaseOwnership first.
    if (m_mxtal) {
      this->releaseOwnership();
    }

    Q_ASSERT(m_mxtal == NULL);
    m_mxtal = parent;

    // Will we need to create new atoms?
    bool addNewAtoms = (atoms.isEmpty() && bonds.isEmpty());

    // Are the lists appropriately sized?
    if (!addNewAtoms) {
      Q_ASSERT_X(atoms.size() == this->numAtoms(), Q_FUNC_INFO,
                 "Size of atoms list must equal numAtoms() or zero.");
      Q_ASSERT_X(bonds.size() == this->numBonds(), Q_FUNC_INFO,
                 "Size of bonds list must equal numBonds() or zero.");
    }

    // Create lists of pointers
    QList<Atom*> newAtoms;
    QList<Bond*> newBonds;
    if (addNewAtoms) {
#if QT_VERSION >= 0x040700
      // Reserve space
      newAtoms.reserve(numAtoms());
      newBonds.reserve(numBonds());
#endif

      // Create atoms
      for (int i = 0; i < this->numAtoms(); ++i) {
        newAtoms.append(m_mxtal->addAtom());
      }

      // Create bonds
      for (int i = 0; i < this->numAtoms(); ++i) {
        newBonds.append(m_mxtal->addBond());
      }
    }
    else { // !addNewAtoms
      newAtoms = atoms;
      newBonds = bonds;
    }

    // Copy atomic information over
    // Lookup table: key = old id, val = new id.
    QHash<unsigned long, unsigned long> atomIdLUT;
    for (int i = 0; i < newAtoms.size(); ++i) {
      Atom *oldAtom = m_atoms.at(i);
      Atom *newAtom = newAtoms.at(i);

      atomIdLUT.insert(oldAtom->id(), newAtom->id());

      *newAtom = *oldAtom;
    }

    // Copy bond info
    for (int i = 0; i < newBonds.size(); ++i) {
      Bond *oldBond = m_bonds.at(i);
      Bond *newBond = newBonds.at(i);

      *newBond = *oldBond;

      unsigned long newBegId = atomIdLUT.value(oldBond->beginAtomId());
      unsigned long newEndId = atomIdLUT.value(oldBond->endAtomId());

      newBond->setBegin(m_mxtal->atomById(newBegId));
      newBond->setEnd(  m_mxtal->atomById(newEndId));
    }

    // Delete old atoms/bonds
    m_dummyParent->clear();

    // Update lists
    m_atoms = newAtoms;
    m_bonds = newBonds;

  }

  void SubMolecule::releaseOwnership()
  {
    if (m_mxtal == NULL) {
      // Nothing to do.
      return;
    }

    m_mxtal = NULL;
    this->m_dummyParent->clear();

    // Create lists of pointers
    QList<Atom*> newAtoms;
    QList<Bond*> newBonds;

#if QT_VERSION >= 0x040700
    // Reserve space
    newAtoms.reserve(numAtoms());
    newBonds.reserve(numBonds());
#endif

    // Create atoms
    for (int i = 0; i < this->numAtoms(); ++i) {
      newAtoms.append(this->m_dummyParent->addAtom());
    }

    // Create bonds
    for (int i = 0; i < this->numBonds(); ++i) {
      newBonds.append(this->m_dummyParent->addBond());
    }

    // Copy atomic information over
    // Lookup table: key = old id, val = new index.
    QHash<unsigned long, unsigned long> atomIdLUT;
    for (int i = 0; i < newAtoms.size(); ++i) {
      Atom *oldAtom = m_atoms.at(i);
      Atom *newAtom = newAtoms.at(i);

      *newAtom = *oldAtom;

      atomIdLUT.insert(oldAtom->id(), i);
    }

    // Copy bond info
    for (int i = 0; i < newBonds.size(); ++i) {
      Bond *oldBond = m_bonds.at(i);
      Bond *newBond = newBonds.at(i);

      *newBond = *oldBond;

      newBond->setBegin(newAtoms[atomIdLUT.value(oldBond->beginAtomId())]);
      newBond->setEnd(newAtoms[atomIdLUT.value(oldBond->endAtomId())]);
    }

    // Update lists
    m_atoms = newAtoms;
    m_bonds = newBonds;

  }

  void SubMolecule::wrapAtomsIntoUnitCell()
  {
    Q_ASSERT_X(this->m_mxtal != NULL, Q_FUNC_INFO,
               "Need to specify unit cell basis if no MolecularXtal owns "
               "the submolecule.");
    const Eigen::Matrix3d rowVectors (
          OB2Eigen(this->m_mxtal->OBUnitCell()->GetCellMatrix()));
    this->wrapAtomsIntoUnitCell(rowVectors);
  }

  void SubMolecule::wrapAtomsIntoUnitCell(const Eigen::Matrix3d &rowVectors)
  {
    const Eigen::Matrix3d cart (rowVectors.transpose());
    const Eigen::Matrix3d frac (cart.inverse());
    Eigen::Vector3d tmp;

    foreach (Atom *atom, this->m_atoms) {
      tmp = *atom->pos();
      tmp = frac * tmp;
      if ((tmp.x() = fmod(tmp.x(), 1.0)) < 0.0) tmp.x() += 1.0;
      if ((tmp.y() = fmod(tmp.y(), 1.0)) < 0.0) tmp.y() += 1.0;
      if ((tmp.z() = fmod(tmp.z(), 1.0)) < 0.0) tmp.z() += 1.0;
      tmp = cart * tmp;
      atom->setPos(tmp);
    }
  }

  void SubMolecule::align(const Eigen::Vector3d &normal,
                          const Eigen::Vector3d &farvec)
  {
    const Eigen::Vector3d oldNormal = this->normal().normalized();
    const Eigen::Vector3d oldFarvec = this->farthestAtomVector().normalized();

    // Occasionally floating point round-off makes the argument of the acos
    // below fall slightly outside of [-1,1], creating a Nan that will blow up
    // the coordinates. Check/correct that here.
    const double angleCosine = oldNormal.dot(normal);
    double angle = 0;
    if (angleCosine > 1.0)
      angle = 0;
    else if (angleCosine < -1.0)
      angle = M_PI;
    else
      angle = acos(angleCosine);

    // Align normals first
    const Eigen::AngleAxisd normalRot (angle, oldNormal.cross(normal));


    //  Get two vectors in the plane of the submol
    const Eigen::Vector3d planeVec1 = normal.unitOrthogonal();
    const Eigen::Vector3d planeVec2 = normal.cross(planeVec1);

    // Then align farvecs' projections onto the submol plane
    const Eigen::Vector3d farpro (
          (planeVec1.dot(farvec)*planeVec1 +
           planeVec2.dot(farvec)*planeVec2).normalized());
    const Eigen::Vector3d oldFarpro (
          (planeVec1.dot(oldFarvec)*planeVec1 +
           planeVec2.dot(oldFarvec)*planeVec2).normalized());

    // Occasionally floating point round-off makes the argument of the acos
    // below fall slightly outside of [-1,1], creating a Nan that will blow up
    // the coordinates. Check/correct that here.
    const double fangleCosine = oldFarpro.dot(farpro);
    double fangle = 0;
    if (fangleCosine > 1.0)
      fangle = 0;
    else if (fangleCosine < -1.0)
      fangle = M_PI;
    else
      fangle = acos(fangleCosine);

    const Eigen::AngleAxisd farvecRot (fangle, oldFarpro.cross(farpro));

    // Remember that operators are applied from right to left
    this->rotate(farvecRot * normalRot);
  }

  void SubMolecule::rotate(const Eigen::AngleAxisd &rot) const
  {
    Eigen::Vector3d pos;
    Eigen::Transform3d transform;
    const Eigen::Vector3d origin (this->center());
    transform.setIdentity();
    transform.pretranslate(-origin);
    transform.prerotate(rot);
    transform.pretranslate(origin);
    for (QList<Atom*>::const_iterator it = m_atoms.constBegin(),
         it_end = m_atoms.constEnd(); it != it_end; ++it) {
      (*it)->setPos( transform * (*(*it)->pos()));
    }
  }

  void SubMolecule::rotate(double angle, const Eigen::Vector3d &axis) const
  {
    this->rotate(Eigen::AngleAxisd(angle, axis.normalized()));
  }

  void SubMolecule::rotate(double angle, const double axis[3]) const
  {
    this->rotate(Eigen::AngleAxisd(angle, Eigen::Vector3d(axis).normalized()));
  }

  void SubMolecule::rotate(double angle,
                           double axis_x, double axis_y, double axis_z) const
  {
    this->rotate(Eigen::AngleAxisd(angle, Eigen::Vector3d(axis_x, axis_y,
                                                          axis_z).normalized()));
  }

  // Translate the atoms in cartesian space:
  void SubMolecule::translate(const Eigen::Vector3d &trans) const
  {
    Eigen::Vector3d pos;
    for (QList<Atom*>::const_iterator it = m_atoms.constBegin(),
         it_end = m_atoms.constEnd(); it != it_end; ++it) {
      pos = *(*it)->pos();
      pos += trans;
      (*it)->setPos(pos);
    }
  }

  void SubMolecule::translate(const double trans[3]) const
  {
    this->translate(Eigen::Vector3d(trans));
  }

  void SubMolecule::translate(double x, double y, double z) const
  {
    this->translate(Eigen::Vector3d(x, y, z));
  }

  // Translate the atoms in fractional space:
  void SubMolecule::translateFrac(const Eigen::Vector3d &trans) const
  {
    Q_ASSERT_X(m_mxtal != NULL, Q_FUNC_INFO, "translateFrac called on a "
               "SubMolecule that does not have an associated MolecularXtal.");

    const Eigen::Vector3d cartTrans (m_mxtal->fracToCart(trans));
    this->translate(cartTrans);
  }

  void SubMolecule::translateFrac(const double trans[3]) const
  {
    this->translateFrac(Eigen::Vector3d(trans));
  }

  void SubMolecule::translateFrac(double x, double y, double z) const
  {
    this->translateFrac(Eigen::Vector3d(x, y, z));
  }

  void SubMolecule::assertCoherentBondLengths(const Eigen::Matrix3d &rowVectors,
                                              const SubMolecule *ref, double tol)
  {
    Q_ASSERT_X(this->numBonds() == ref->numBonds(), Q_FUNC_INFO,
               "Number of bonds has changed.");
    SubMolecule *clone = this->getCoherentClone();
    clone->makeCoherent(NULL, rowVectors);
    for (int i = 0; i < clone->numBonds(); ++i) {
      Q_ASSERT_X(fabs(clone->bond(i)->length() - ref->bond(i)->length()) < tol,
                 Q_FUNC_INFO, "Bond length changed.");
    }
    delete clone;
  }

} // end namespace XtalOpt
