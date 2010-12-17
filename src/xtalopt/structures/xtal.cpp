/**********************************************************************
  Xtal - Wrapper for Structure to ease work with crystals.

  Copyright (C) 2009-2010 by David C. Lonie

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

#include <globalsearch/macros.h>

#include <openbabel/generic.h>
#include <openbabel/forcefield.h>

#include <QtCore/QFile>
#include <QtCore/QDebug>
#include <QtCore/QRegExp>
#include <QtCore/QStringList>

extern "C" {
#include <spglib/spglib.h>
}

using namespace std;
using namespace OpenBabel;
using namespace Avogadro;
using namespace Eigen;

#define STABLE_COMP_TOL 1e-5
#define SWAP(v1, v2, tmp) tmp=v1;v1=v2;v2=tmp;

// Verbose printing of iterative information during Niggli reduction
//#define NIGGLI_DEBUG
#ifdef NIGGLI_DEBUG
#define NIGGLI_PRINT_HEADER                      \
  printf("%3s %1s %5s %5s %5s %5s %5s %5s\n",    \
         "Itr", "S", "A", "B", "C", "zeta", "eta", "xi");
#define NIGGLI_PRINT(iter, step)                 \
  printf("%3d %1d %5.0f %5.0f %5.0f %5.0f %5.0f %5.0f\n",    \
         iter, step, A, B, C, zeta, eta, xi);

#else

#define NIGGLI_PRINT_HEADER void();
#define NIGGLI_PRINT(iter, step) void();

#endif

namespace XtalOpt {

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
    for (int i = 0; i < atomList.size(); i++)
      atomList.at(i)->setPos(fracToCart(fracCoordsList.at(i)));
  }

  // Helper functions for comparing numbers with a tolerance
  inline bool stable_lt(const double &v1, const double &v2, const double &prec = STABLE_COMP_TOL)
  {
    return ( v1 < (v2 - prec) );
  }

  inline bool stable_gt(const double &v1, const double &v2, const double &prec = STABLE_COMP_TOL)
  {
    return ( v2 < (v1 - prec) );
  }

  inline bool stable_eq(const double &v1, const double &v2, const double &prec = STABLE_COMP_TOL)
  {
    return (!(stable_lt(v1,v2,prec) ||
              stable_gt(v1,v2,prec) ) );
  }

  inline bool stable_neq(const double &v1, const double &v2, const double &prec = STABLE_COMP_TOL)
  {
    return (!(stable_eq(v1,v2,prec)));
  }

  inline bool stable_leq(const double &v1, const double &v2, const double &prec = STABLE_COMP_TOL)
  {
    return (!stable_gt(v1, v2, prec));
  }

  inline bool stable_geq(const double &v1, const double &v2, const double &prec = STABLE_COMP_TOL)
  {
    return (!stable_lt(v1, v2, prec));
  }

  inline double sign(const double &v)
  {
    // consider 0 to be positive
    if (v >= 0) return 1.0;
    else return -1.0;
  }

  bool Xtal::niggliReduce(double *a_, double *b_, double *c_,
                          double *alpha_, double *beta_, double *gamma_,
                          const unsigned int iterations)
  {
    // convert deg->rad
    double a     = *a_;
    double b     = *b_;
    double c     = *c_;
    double alpha = (*alpha_) * DEG_TO_RAD;
    double beta  = (*beta_)  * DEG_TO_RAD;
    double gamma = (*gamma_) * DEG_TO_RAD;

    // For swapping
    double tmp;

    // Characteristic (step 0)
    double A    = a*a;
    double B    = b*b;
    double C    = c*c;
    double zeta = 2*b*c*cos(alpha);
    double eta  = 2*a*c*cos(beta);
    double xi   = 2*a*b*cos(gamma);

    // Return value
    bool ret = false;

    // comparison tolerance
    double tol = STABLE_COMP_TOL * pow(a * b * c, 1.0/3.0);

    NIGGLI_PRINT_HEADER;
    NIGGLI_PRINT(0,0);

    for (int iter = 0; iter < iterations; iter++) {
      // Step 1:
      if (
          stable_gt(A, B, tol)
          || (
              stable_eq(A, B, tol)
              &&
              stable_gt(fabs(zeta), fabs(eta), tol)
              )
          ) {
        SWAP(A, B, tmp);
        SWAP(zeta, eta, tmp);
        NIGGLI_PRINT(iter+1,1);
      }

      // Step 2:
      if (
          stable_gt(B, C, tol)
          || (
              stable_eq(B, C, tol)
              &&
              stable_gt(fabs(eta), fabs(xi), tol)
              )
          ) {
        SWAP(B, C, tmp);
        SWAP(eta, xi, tmp);
        NIGGLI_PRINT(iter+1,2);
        continue;
      }

      double zetaEtaXi = zeta*eta*xi;
      // Step 3:
      if (stable_gt(zetaEtaXi, 0, tol)) {
        zeta = fabs(zeta);
        eta  = fabs(eta);
        xi   = fabs(xi);
        NIGGLI_PRINT(iter+1,3);
      }

      // Step 4:
      if (stable_leq(zetaEtaXi, 0, tol)) {
        zeta = -fabs(zeta);
        eta  = -fabs(eta);
        xi   = -fabs(xi);
        NIGGLI_PRINT(iter+1,4);
      }

      // Step 5:
      if (stable_gt(fabs(zeta), B, tol)
          || (stable_eq(zeta, B, tol)
              && stable_lt(2*eta, xi, tol)
              )
          || (stable_eq(zeta, -B, tol)
              && stable_lt(xi, 0, tol)
              )
          ) {
        double signZeta = sign(zeta);
        C    = B + C - zeta*signZeta;
        eta  = eta - xi*signZeta;
        zeta = zeta - 2*B*signZeta;
        NIGGLI_PRINT(iter+1,5);
        continue;
      }

      // Step 6:
      if (stable_gt(fabs(eta), A, tol)
          || (stable_eq(eta, A, tol)
              && stable_lt(2*zeta, xi, tol)
              )
          || (stable_eq(eta, -A, tol)
              && stable_lt(xi, 0, tol)
              )
          ) {
        double signEta = sign(eta);
        C    = A + C - eta*signEta;
        zeta = zeta - xi*signEta;
        eta  = eta - 2*A*signEta;
        NIGGLI_PRINT(iter+1,6);
        continue;
      }

      // Step 7:
      if (stable_gt(fabs(xi), A, tol)
          || (stable_eq(xi, A, tol)
              && stable_lt(2*zeta, eta, tol)
              )
          || (stable_eq(xi, -A, tol)
              && stable_lt(eta, 0, tol)
              )
          ) {
        double signXi = sign(xi);
        B    = A + B - xi*signXi;
        zeta = zeta - eta*signXi;
        xi   = xi - 2*A*signXi;
        NIGGLI_PRINT(iter+1,7);
        continue;
      }

      // Step 8:
      double sumAllButC = A + B + zeta + eta + xi;
      if (stable_lt(sumAllButC, 0, tol)
          || (stable_eq(sumAllButC, 0, tol)
              && stable_gt(2*(A+eta)+xi, 0, tol)
              )
          ) {
        C    = sumAllButC + C;
        zeta = 2*B + zeta + xi;
        eta  = 2*A + eta + xi;
        NIGGLI_PRINT(iter+1,8);
        continue;
      }

      // Done!
      ret = true;
      NIGGLI_PRINT(iter+1,0);
      break;
    }

    // Update values
    if (ret == true) {
      (*a_) = sqrt(A);
      (*b_) = sqrt(B);
      (*c_) = sqrt(C);
      (*alpha_) = acos(zeta / (2*(*b_)*(*c_))) * RAD_TO_DEG;
      (*beta_)  = acos(eta  / (2*(*a_)*(*c_))) * RAD_TO_DEG;
      (*gamma_) = acos(xi   / (2*(*a_)*(*b_))) * RAD_TO_DEG;
    }

    return ret;
  }

  bool Xtal::isNiggliReduced() const
  {
    // cache params
    double a     = getA();
    double b     = getB();
    double c     = getC();
    double alpha = getAlpha();
    double beta  = getBeta();
    double gamma = getGamma();

    return Xtal::isNiggliReduced(a, b, c, alpha, beta, gamma);
  }

  bool Xtal::isNiggliReduced(const double a, const double b, const double c,
                             const double alpha, const double beta, const double gamma)
  {
    // Calculate characteristic
    double A    = a*a;
    double B    = b*b;
    double C    = c*c;
    double zeta = 2*b*c*cos(alpha);
    double eta  = 2*a*c*cos(beta);
    double xi   = 2*a*b*cos(gamma);

    // comparison tolerance
    double tol = STABLE_COMP_TOL * pow(a * b * c, 1.0/3.0);

    // Check against Niggli conditions (taken from Gruber 1973). The
    // logic of the second comparison is reversed from the paper to
    // simplify the algorithm.
    if (stable_eq(zeta,  B, tol) && stable_gt (xi,   2*eta,  tol)) return false;
    if (stable_eq(eta ,  A, tol) && stable_gt (xi,   2*zeta, tol)) return false;
    if (stable_eq(xi,    A, tol) && stable_gt (eta,  2*zeta, tol)) return false;
    if (stable_eq(zeta, -B, tol) && stable_neq(xi,   0,      tol)) return false;
    if (stable_eq(eta,  -A, tol) && stable_neq(xi,   0,      tol)) return false;
    if (stable_eq(xi,   -A, tol) && stable_neq(eta,  0,      tol)) return false;

    if (stable_eq(zeta+eta+xi+A+B, 0, tol)
        && stable_gt(2*(A+eta)+xi,  0, tol)) return false;

    // all good!
    return true;
  }

  bool Xtal::fixAngles(int attempts)
  {
    // Store checks
    double oldVolume = getVolume();

    // Get rotation matrix
    matrix3x3 rot = cell()->GetOrientationMatrix();

    // Extract cell parameters
    double a_, b_, c_, alpha_, beta_, gamma_;
    double a = a_ = getA();
    double b = b_ = getB();
    double c = c_ = getC();
    double alpha = alpha_ = getAlpha();
    double beta  = beta_  = getBeta();
    double gamma = gamma_ = getGamma();

    // Remove rotation matrix if needed
    if (!rot.isUnitMatrix()) {
      // If so, store the fractional coordinates
      QList<Eigen::Vector3d> fracpos;
      for (int i = 0; i < numAtoms(); i++) {
        fracpos.append(cartToFrac(*atom(i)->pos()));
      }
      // Set the cell using only parameters
      setCellInfo(a,b,c,alpha,beta,gamma);
      // Update atom positions using old frac coords
      for (int i = 0; i < numAtoms(); i++) {
        atom(i)->setPos(fracToCart(fracpos.at(i)));
      }
    }

    // Perform niggli reduction
    if (!niggliReduce(&a, &b, &c, &alpha, &beta, &gamma, attempts)) {
      qDebug() << "Unable to perform cell reduction on Xtal " << getIDString()
               << "( " << a_ << b_ << c_ << alpha_ << beta_ << gamma_ << " )";
      return false;
    }

    // Build update cell with new params
    setCellInfo(a,b,c,alpha,beta,gamma);

    // Wrap atoms into new cell
    wrapAtomsToCell();

    // Check volume
    double newVolume = getVolume();
    Q_ASSERT_X(fabs(oldVolume - newVolume) < 1e-5,
               Q_FUNC_INFO,
               QString("Cell volume changed during niggli reduction for structure %1.\n\
Params: %2 %3 %4 %5 %6 %7\n\
Volumes: old=%8 new=%9")
               .arg(getIDString())
               .arg(a)
               .arg(b)
               .arg(c)
               .arg(alpha)
               .arg(beta)
               .arg(gamma)
               .arg(oldVolume)
               .arg(newVolume)
               .toStdString().c_str());

    // Ensure that the new cell is actually the niggli cell. If not, print warning
    if (!isNiggliReduced()) {
      qWarning() << QString("Niggli-reduction failed for structure %1.\n\
Params: %2 %3 %4 %5 %6 %7")
        .arg(getIDString())
        .arg(a)
        .arg(b)
        .arg(c)
        .arg(alpha)
        .arg(beta)
        .arg(gamma)
        .toStdString().c_str();
    }

    findSpaceGroup();
    return true;
  }

  bool Xtal::operator==(const Xtal &o) const
  {
    // check that the cells are niggli reduced first. If not, warn in debug output
    if (!isNiggliReduced()) {
      qWarning() << "Warning: Structure " << getIDString() <<
        " is be compared but is not Niggli reduced.";
    }
    if (!o.isNiggliReduced()) {
      qWarning() << "Warning: Structure " << o.getIDString() <<
        " is be compared but is not Niggli reduced.";
    }

    // Compare coordinates using the default tolerance
    if (!compareCoordinates(o)) return false;

    // Compare volumes. Tolerance is 1% of this->getVolume()
    double vol = getVolume();
    const double voltol = 0.01 * vol;
    if (fabs(vol - o.getVolume()) > voltol) return false;

    // Compare lattice params
    const double lengthtol = 0.05;
    const double angletol  = 0.1;
    if (fabs(getA() - o.getA()) > lengthtol) return false;
    if (fabs(getB() - o.getB()) > lengthtol) return false;
    if (fabs(getC() - o.getC()) > lengthtol) return false;
    if (fabs(getAlpha() - o.getAlpha()) > angletol) return false;
    if (fabs(getBeta()  - o.getBeta())  > angletol) return false;
    if (fabs(getGamma() - o.getGamma()) > angletol) return false;

    // all good!
    return true;
  }

  struct ComparisonAtom
  {
    unsigned int atomicNumber;
    Eigen::Vector3d pos;
  };

  bool Xtal::compareCoordinates(const Xtal &o, const double tol) const
  {
    double tol2 = tol*tol;

    // Are there any atoms?
    unsigned int thisNumAtoms = this->numAtoms();
    if (thisNumAtoms == 0) {
      if (o.numAtoms() == 0) return true;
      else return false;
    }

    // First ensure that the compositions are the same
    QList<QString> atomSymbols = getSymbols();
    QList<unsigned int> atomCounts = getNumberOfAtomsAlpha();

    if (atomSymbols != o.getSymbols()) return false;
    if (atomCounts  != o.getNumberOfAtomsAlpha()) return false;

    // Now locate the most infrequent species in the structures
    unsigned int min=UINT_MAX, minIndex, current;
    for (unsigned int i = 0; i < atomCounts.size(); i++) {
      current = atomCounts[i];
      if (current < min && current != 0) {
        min = current;
        minIndex = i;
      }
    }

    // Find atomic number of most infrequent species;
    unsigned int lfAtomicNumber =
      OpenBabel::etab.GetAtomicNum(atomSymbols[minIndex].toStdString().c_str());

    // Now build a list of all atoms in this
    ComparisonAtom thisAtoms[thisNumAtoms];
    ComparisonAtom ca;
    unsigned int atomCounter = 0;
    for (QList<Atom*>::const_iterator atm = m_atomList.begin();
         atm != m_atomList.end();
         atm++) {
      ca.atomicNumber = (*atm)->atomicNumber();
      ca.pos = *(*atm)->pos();
      thisAtoms[atomCounter++] = ca;
    }

    // Now build a list of all atoms in a 3x3x3 supercell of other
    unsigned int otherNumAtoms = 27 * thisNumAtoms; // # atoms in supercell
    ComparisonAtom otherAtoms[otherNumAtoms];
    matrix3x3 obmat = o.cell()->GetCellMatrix();
    const Eigen::Vector3d xvec (obmat.Get(0,0),
                                obmat.Get(0,1),
                                obmat.Get(0,2));
    const Eigen::Vector3d yvec (obmat.Get(1,0),
                                obmat.Get(1,1),
                                obmat.Get(1,2));
    const Eigen::Vector3d zvec (obmat.Get(2,0),
                                obmat.Get(2,1),
                                obmat.Get(2,2));
    Eigen::Vector3d tmpTranslation;
    atomCounter = 0;
    for (int dx = -1; dx <= 1; dx++) {
      for (int dy = -1; dy <= 1; dy++) {
        for (int dz = -1; dz <= 1; dz++) {
          // Calc translation vector
          tmpTranslation = dx*xvec;
          tmpTranslation += dy*yvec;
          tmpTranslation += dz*zvec;

          // Add atoms (loop over thisNumAtoms since we already
          // checked composition, and otherNumAtoms counts for the supercell
          for (QList<Atom*>::const_iterator atm = o.m_atomList.begin();
               atm != o.m_atomList.end();
               atm++) {
            ca.atomicNumber = (*atm)->atomicNumber();
            ca.pos = *(*atm)->pos();
            ca.pos += tmpTranslation;
            otherAtoms[atomCounter++] = ca;
          }
        }
      }
    }

    // Prepare for the comparisons. First locate the reference atom in
    // this. It is the first atom of type lfAtomicNumber
    Eigen::Vector3d refTrans;
    for (unsigned int i = 0; i < thisNumAtoms; i++) {
      if (thisAtoms[i].atomicNumber == lfAtomicNumber) {
        refTrans = thisAtoms[i].pos;
        break;
      }
    }

    // Translate all atoms in thisAtoms by the -ref.pos, effectively
    // placing ref at the origin;
    for (unsigned int i = 0; i < thisNumAtoms; i++) {
      thisAtoms[i].pos -= refTrans;
    }

    // Now for the comparisons. Declarations:
    const Eigen::Vector3d *pivotTrans;
    const ComparisonAtom *pivot;
    const ComparisonAtom *thisAtom;
    const ComparisonAtom *otherAtom;
    bool atomMatched;
    double dx, dy, dz;

    // First locate a pivot atom in otherAtomList of type lfAtomicNumber
    for (unsigned int pivotIndex = 0; pivotIndex < otherNumAtoms; pivotIndex++) {
      pivot = &otherAtoms[pivotIndex];
      if (pivot->atomicNumber != lfAtomicNumber) continue;
      pivotTrans = &pivot->pos;

      // Now that we have a pivot, compare all atoms in thisAtoms with
      // all atoms in otherAtoms after translating other's atoms by -pivotTrans
      for (unsigned int thisAtomIndex = 0;
           thisAtomIndex < thisNumAtoms;
           thisAtomIndex++) {
        thisAtom = &thisAtoms[thisAtomIndex];
        atomMatched = false;
        for (unsigned int otherAtomIndex = 0;
             otherAtomIndex < otherNumAtoms;
             otherAtomIndex++) {
          otherAtom = &otherAtoms[otherAtomIndex];

          // compare atomic numbers
          if (otherAtom->atomicNumber != thisAtom->atomicNumber) continue;
          // compare positions
          dx = thisAtom->pos.x() - otherAtom->pos.x() + pivotTrans->x();
          dy = thisAtom->pos.y() - otherAtom->pos.y() + pivotTrans->y();
          dz = thisAtom->pos.z() - otherAtom->pos.z() + pivotTrans->z();
          if (fabs( dx*dx+dy*dy+dz*dz ) < tol2) {
            atomMatched = true;
            break;
          }
        }
        if (!atomMatched) break; // Find new pivot
      }
      if (atomMatched) { // All atoms have a match, success!
        return true;
      }
    }
    // All pivots failed; coordinates do not match
    return false;
  }

  OpenBabel::OBUnitCell* Xtal::cell() const
  {
    return OBUnitCell();
  }


  bool Xtal::addAtomRandomly(uint atomicNumber, double minIAD, double maxIAD, int maxAttempts, Atom **atom) {
    INIT_RANDOM_GENERATOR();
    Q_UNUSED(maxIAD);

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
        double x = RANDDOUBLE();
        double y = RANDDOUBLE();
        double z = RANDDOUBLE();

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
    atom = &atm;
    Eigen::Vector3d pos (cartCoords[0],cartCoords[1],cartCoords[2]);
    (*atom)->setPos(pos);
    (*atom)->setAtomicNumber(static_cast<int>(atomicNumber));
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

    shortest = fabs((v1-v2).norm());
    double distance;

    // Find shortest distance
    for (int i = 0; i < atomList.size(); i++) {
      v1 = atomPositions.at(i);
      for (int j = i+1; j < atomList.size(); j++) {
        v2 = atomPositions.at(j);
        // Intercell
        distance = fabs((v1-v2).norm());
        if (distance < shortest) shortest = distance;
        // Intracell
        for (int vecInd = 0; vecInd < uVecs.size(); vecInd++) {
          distance = fabs(((v1+uVecs.at(vecInd))-v2).norm());
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

    shortest = fabs((v1-v2).norm());
    double distance;

    // Find shortest distance
    for (int j = 0; j < atomList.size(); j++) {
      v2 = atomPositions.at(j);
      // Intercell
      distance = fabs((v1-v2).norm());
      if (distance < shortest) shortest = distance;
      // Intracell
      for (int vecInd = 0; vecInd < uVecs.size(); vecInd++) {
        distance = fabs(((v2+uVecs.at(vecInd))-v1).norm());
        if (distance < shortest) shortest = distance;
      }
    }
    return true;
  }

  bool Xtal::getIADHistogram(QList<double> * distance,
                             QList<double> * frequency,
                             double min, double max, double step,
                             Atom *atom) const {

    if (min > max && step > 0) {
      qWarning() << "Xtal::getIADHistogram: min cannot be greater than max!";
      return false;
    }
    if (step < 0 || step == 0) {
      qWarning() << "Xtal::getIADHistogram: invalid step size:" << step;
      return false;
    }

    // Populate distance list
    distance->clear();
    frequency->clear();
    double val = min;
    do {
      distance->append(val);
      frequency->append(0);
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
          diff = fabs((v1-v2).norm());
          for (int k = 0; k < distance->size(); k++) {
            double radius = distance->at(k);
            if (fabs(diff-radius) < step/2) {
              (*frequency)[k]++;
            }
          }
          // Intracell
          for (int vecInd = 0; vecInd < uVecs.size(); vecInd++) {
            diff = fabs(((v1+uVecs.at(vecInd))-v2).norm());
            for (int k = 0; k < distance->size(); k++) {
              double radius = distance->at(k);
              if (fabs(diff-radius) < step/2) {
                (*frequency)[k]++;
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
        diff = fabs((v1-v2).norm());
        for (int k = 0; k < distance->size(); k++) {
          double radius = distance->at(k);
          if (diff != 0 && fabs(diff-radius) < step/2) {
            (*frequency)[k]++;
          }
        }
        // Intracell
        for (int vecInd = 0; vecInd < uVecs.size(); vecInd++) {
          diff = fabs(((v1+uVecs.at(vecInd))-v2).norm());
          for (int k = 0; k < distance->size(); k++) {
            double radius = distance->at(k);
            if (fabs(diff-radius) < step/2) {
              (*frequency)[k]++;
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

  QHash<QString, QVariant> Xtal::getFingerprint()
  {
    QHash<QString, QVariant> fp = Structure::getFingerprint();
    fp.insert("volume", getVolume());
    fp.insert("spacegroup", getSpaceGroupNumber());
    return fp;
  }

  QString Xtal::getResultsEntry() const
  {
    QString status;
    switch (getStatus()) {
    case Optimized:
      status = "Optimized";
      break;
    case Killed:
    case Removed:
      status = "Killed";
      break;
    case Duplicate:
      status = "Duplicate";
      break;
    case Error:
      status = "Error";
      break;
    case StepOptimized:
    case WaitingForOptimization:
    case InProcess:
    case Empty:
    case Updating:
    case Submitted:
    default:
      status = "In progress";
      break;
    }
    return QString("%1 %2 %3 %4 %5 %6")
      .arg(getRank(), 6)
      .arg(getGeneration(), 6)
      .arg(getIDNumber(), 6)
      .arg(getEnthalpy(), 10)
      .arg(m_spgSymbol, 10)
      .arg(status, 11);
  };


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
    // Check that the precision is reasonable
    if (prec < 1e-5) {
      qWarning() << "Xtal::findSpaceGroup called with a precision of "
                 << prec << ". This is likely an error. Resetting prec to "
                 << 0.05 << ".";
      prec = 0.05;
    }

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
    double (*positions)[3] = new double[num][3];
    int *types = new int[num];
    QList<Atom*> atomList = atoms();
    Eigen::Vector3d fracCoords;
    for (int i = 0; i < atomList.size(); i++) {
      fracCoords        = cartToFrac(*(atomList.at(i)->pos()));
      types[i]          = atomList.at(i)->atomicNumber();
      positions[i][0]   = fracCoords.x();
      positions[i][1]   = fracCoords.y();
      positions[i][2]   = fracCoords.z();
    }

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
    m_spgNumber = spg_get_international(symbol,
                                        lattice,
                                        positions,
                                        types,
                                        num, prec);

    delete [] positions;
    delete [] types;

    // Fail if m_spgNumber is still 0
    if (m_spgNumber == 0) {
      //qDebug() << "Xtal::findSpaceGroup( " << prec << " ): spglib unable to find spacegroup!";
      return;
    }

    // Set and clean up the symbol string
    m_spgSymbol = QString(symbol);
    m_spgSymbol.remove(" ");
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

  Xtal* Xtal::POSCARToXtal(const QString &poscar)
  {
    QTextStream ps (&const_cast<QString &>(poscar));
    QStringList sl;
    vector3 v1, v2, v3, pos;
    Xtal *xtal = new Xtal;

    ps.readLine(); // title
    float scale = ps.readLine().toFloat(); // Scale factor
    sl = ps.readLine().split(QRegExp("\\s+"), QString::SkipEmptyParts); // v1
    v1.x() = sl.at(0).toFloat() * scale;
    v1.y() = sl.at(1).toFloat() * scale;
    v1.z() = sl.at(2).toFloat() * scale;

    sl = ps.readLine().split(QRegExp("\\s+"), QString::SkipEmptyParts); // v2
    v2.x() = sl.at(0).toFloat() * scale;
    v2.y() = sl.at(1).toFloat() * scale;
    v2.z() = sl.at(2).toFloat() * scale;

    sl = ps.readLine().split(QRegExp("\\s+"), QString::SkipEmptyParts); // v3
    v3.x() = sl.at(0).toFloat() * scale;
    v3.y() = sl.at(1).toFloat() * scale;
    v3.z() = sl.at(2).toFloat() * scale;

    xtal->setCellInfo(v1, v2, v3);

    sl = ps.readLine().split(QRegExp("\\s+"), QString::SkipEmptyParts); // atom types
    unsigned int numAtomTypes = sl.size();
    QList<unsigned int> atomCounts;
    for (int i = 0; i < numAtomTypes; i++) {
      atomCounts.append(sl.at(i).toUInt());
    }

    // TODO this will assume fractional coordinates. VASP can use cartesian!
    ps.readLine(); // direct or cartesian
    // Atom coords begin
    Atom *atom;
    for (unsigned int i = 0; i < numAtomTypes; i++) {
      for (unsigned int j = 0; j < atomCounts.at(i); j++) {
        // Actual identity of the atoms doesn't matter for the symmetry
        // test. Just use (i+1) as the atomic number.
        atom = xtal->addAtom();
        atom->setAtomicNumber(i+1);
        // Get coords
        sl = ps.readLine().split(QRegExp("\\s+"), QString::SkipEmptyParts); // coords
        Eigen::Vector3d pos;
        pos.x() = sl.at(0).toDouble();
        pos.y() = sl.at(1).toDouble();
        pos.z() = sl.at(2).toDouble();
        atom->setPos(pos);
      }
    }

    return xtal;
  }

  Xtal* Xtal::POSCARToXtal(QFile *file)
  {
    QString poscar;
    file->open(QFile::ReadOnly);
    poscar = file->readAll();
    file->close();
    return POSCARToXtal(poscar);
  }

} // end namespace XtalOpt

//#include "xtal.moc"
