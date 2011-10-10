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

#ifndef SUBMOLECULE_H
#define SUBMOLECULE_H

#include <avogadro/primitive.h>

#include <QtCore/QVector>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <limits.h>

namespace Avogadro {
  class Atom;
  class Bond;
  class Molecule;
}

namespace XtalOpt {
  class SubMoleculeSource;
  class MolecularXtal;

  class SubMolecule : public Avogadro::Primitive
  {
    Q_OBJECT

  public:
    virtual ~SubMolecule();

    double radius() const;
    Avogadro::Atom * farthestAtom() const;
    const Eigen::Vector3d * farthestAtomPos() const; // cartesian coordinate
    Eigen::Vector3d farthestAtomVector() const  // vector from center()
    {
      return *this->farthestAtomPos() - center();
    }
    Eigen::Vector3d center() const;
    // Fit a plane to the atomic positions, return the plane's normal
    Eigen::Vector3d normal() const;

    // Get a list of spatially coherent cartesian coordinates for this
    // submolecule. The order is the same as the index into the list returned
    // by atoms()
    QVector<Eigen::Vector3d> getCoherentCoordinates(
        Avogadro::Atom *req_start = NULL,
        const Eigen::Matrix3d &req_cellRowMatrix = Eigen::Matrix3d::Zero()) const;

    // Translate atoms so that all atoms in the molecule are spatially
    // coherent by selecting atom images that are the closest together. If
    // provided, "start" indicates an to use as a starting point.
    void makeCoherent(Avogadro::Atom *start = NULL,
                      const Eigen::Matrix3d &cellRowMatrix =
                      Eigen::Matrix3d::Zero()) const;

    // Get a spatially coherent unit of all atoms in the submolecule.
    // If provided, "start" indicates an to use as a starting point.
    // Creates a new SubMolecule that must be deleted.
    SubMolecule * getCoherentClone(Avogadro::Atom *start = NULL) const;

    // Verify that bonds aren't excessively long. True if ok, false otherwise.
    // Max dist is (1.2 * (r_c1 + r_c2))
    bool verifyBonds(const Eigen::Matrix3d &cellRowMatrix =
        Eigen::Matrix3d::Zero()) const;

    // If a MolecularXtal owns the submolecule, the pointer lists point to
    // the objects (Atoms and Bonds) in the MolecularXtal. If there is no
    // ownership, the atoms are owned by the SubMolecule.
    //
    // The lists in the takeOwnership function should contain atoms and bonds
    // that are to be used to form the submolecule. They must equal numAtoms
    // and numBonds, or both be empty. If empty, new atoms are added to the
    // structure. After ownership is taken, the atoms() and bonds() methods
    // will return lists that point to the MoleculalXtal's atoms and bonds,
    // which may be modified.
    //
    // @pre atoms.size() == numAtoms() || atoms.size() == 0
    // @pre bonds.size() == numAtoms() || bonds.size() == 0
    void takeOwnership(MolecularXtal *parent,
                       const QList<Avogadro::Atom*> &atoms,
                       const QList<Avogadro::Bond*> &bonds);
    // Releasing ownership creates new atoms to take the place of the old
    // pointers. This will NOT remove the atoms/bonds from the current parent.
    void releaseOwnership();

    MolecularXtal * mxtal() {return m_mxtal;}

    // Identify the submolecule source
    unsigned long sourceId() const {return m_sourceId;}

    int numAtoms() const {return m_atoms.size();}
    Avogadro::Atom * atom(int i) {return m_atoms.at(i);}
    const Avogadro::Atom * atom(int i) const {return m_atoms.at(i);}
    QList<Avogadro::Atom*> atoms() {return m_atoms;}
    const QList<Avogadro::Atom*> & atoms() const {return m_atoms;}
    int indexOfAtom(Avogadro::Atom *a) const {return m_atoms.indexOf(a);}

    int numBonds() const {return m_bonds.size();}
    Avogadro::Bond * bond(int i) {return m_bonds.at(i);}
    const Avogadro::Bond * bond(int i) const {return m_bonds.at(i);}
    QList<Avogadro::Bond*> bonds() {return m_bonds;}
    const QList<Avogadro::Bond*> & bonds() const {return m_bonds;}
    int indexOfBond(Avogadro::Bond *b) const {return m_bonds.indexOf(b);}

    void wrapAtomsIntoUnitCell();
    void wrapAtomsIntoUnitCell(const Eigen::Matrix3d &rowVectors);

  public slots:
    // Translate so that the geometric center of this is at newPos
    void setCenter(const Eigen::Vector3d &newPos)
    {
      this->translate(newPos - this->center());
    }

    // Align this to the specified normal and farthestAtomVector. Both must be
    // normalized.
    void align(const Eigen::Vector3d &normal, const Eigen::Vector3d &farvec);

    // Rotate the atoms about this->center(): Angles in radian.
    // Note: It is generally a Good Idea to call makeCoherent first!
    void rotate(const Eigen::AngleAxisd &rot) const;
    void rotate(double angle, const Eigen::Vector3d &axis) const;
    void rotate(double angle, const double axis[3]) const;
    void rotate(double angle,
                double axis_x, double axis_y, double axis_z) const;

    // Translate the atoms in cartesian space:
    void translate(const Eigen::Vector3d &trans) const;
    void translate(const double trans[3]) const;
    void translate(double x, double y, double z) const;

    // Translate the atoms in fractional space:
    void translateFrac(const Eigen::Vector3d &trans) const;
    void translateFrac(const double trans[3]) const;
    void translateFrac(double x, double y, double z) const;

    // Debugging:
    // Assert that the bonds in this are the same length as those in ref.
    void assertCoherentBondLengths(const Eigen::Matrix3d &rowVectors,
                                   const SubMolecule *ref, double tol = 1e-3);

    friend class SubMoleculeSource;
    friend class MolecularXtal;

  protected:
    SubMolecule(QObject *parent = 0);

    unsigned long m_sourceId;
    void setSourceId(unsigned long i) {m_sourceId = i;}

    // This molecule is the dummy parent for the atoms when the submolecule
    // is unowned (otherwise positions, etc of the atoms are invalid).
    // This instance should not be changed.
    MolecularXtal *m_dummyParent;

    // The molecule that this submolecule belongs to. May be NULL -- in that
    // case, the submolecule's atoms and bonds actually belong to
    // m_dummyParent.
    // To set the molecule on an already-constructed SubMolecule (say, from a
    // SubMoleculeSource), use takeOwnership().
    MolecularXtal *m_mxtal;

    QList<Avogadro::Atom*> m_atoms;
    QList<Avogadro::Bond*> m_bonds;
  };

} // end namespace XtalOpt

#endif
