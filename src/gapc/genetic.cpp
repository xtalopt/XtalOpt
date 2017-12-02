/**********************************************************************
  GAPC -- A genetic algorithm for protected clusters

  Copyright (C) 2009-2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <gapc/genetic.h>

#include <globalsearch/macros.h>

#include <QDebug>
#include <QFile>

#include <vector>

// Debugging:
//#define DUMP_STRUCTURES
#undef DUMP_STRUCTURES

using namespace std;
using namespace OpenBabel;
using namespace Avogadro;
using namespace Eigen;

namespace GAPC {

// helper functions
//
// Create a rotation matrix. x, y, z are the components of a vector
// along the rotation axis, and theta is the angle. Omit theta to
// create a random rotation around a specified axis, or omit all for
// a fully random rotation matrix.
//
// theta is in radians.
inline Matrix3d createRotationMatrix(double x = 0.0, double y = 0.0,
                                     double z = 0.0, double theta = 0.0)
{
  INIT_RANDOM_GENERATOR();
  // This function builds a rotation matrix:
  //
  // [ [ tx^2+c txy-sz txz+sy ]
  //   [ txy+sz ty^2+c txy-sx ]
  //   [ txz-sy tyz+sx tz^2+c ] ]
  //
  // where x,y,z is the unit vector along the axis of rotation, and
  //
  // c = cos(theta)
  // s = sin(theta)
  // t = 1-cos(theta)
  //
  // (taken from
  // http://www.euclideanspace.com/maths/geometry/rotations/conversions/angleToMatrix/index.htm)
  //
  // First, generate x,y,z randomly and normalize to get the
  // rotation axis:
  if (x == 0.0 && y == 0.0 && z == 0.0) {
    x = RANDDOUBLE();
    y = RANDDOUBLE();
    z = RANDDOUBLE();
  }
  // Normalize
  double length = sqrt(x * x + y * y + z * z);
  x /= length;
  y /= length;
  z /= length;

  // Now randomly generate theta (on [0,2*pi]), c, s, and t
  if (theta == 0.0) {
    theta = RANDDOUBLE() * 2 * M_PI;
  }
  double c = cos(theta);
  double s = sin(theta);
  double t = 1 - c;

  // Actually build rotation matrix
  Matrix3d m;
  m << t * x * x + c, t * x * y - s * z, t * x * z + s * y, t * x * y + s * z,
    t * y * y + c, t * x * y - s * x, t * x * z - s * y, t * y * z + s * x,
    t * z * z + c;
  return m;
}

// rotate the vector of coordinates by the rotation matrix rot
inline vector<Vector3d> rotateCoordinates(const vector<Vector3d>& coords,
                                          Matrix3d rot)
{
  vector<Vector3d> newCoords;
  // Eigen::Vector3d is a column vector, so transpose before and
  // after transforming them.
  for (int i = 0; i < coords.size(); i++) {
    newCoords.push_back((coords.at(i).transpose() * rot).transpose());
  }
  return newCoords;
}

#ifdef DUMP_STRUCTURES
// Write an xyz file of the passed structure
inline void dumpXYZ(const QString& filename, const QList<Eigen::Vector3d*>& vs,
                    const QList<int>& ans)
{
  QFile f(filename);
  f.open(QIODevice::WriteOnly | QIODevice::Text);
  QTextStream out(&f);
  out << vs.size() << endl << endl;
  for (int i = 0; i < vs.size(); i++) {
    out << QString("%1 %2 %3 %4\n")
             .arg(ans.at(i), 3)
             .arg(vs.at(i)->x(), 5, 'g')
             .arg(vs.at(i)->y(), 5, 'g')
             .arg(vs.at(i)->z(), 5, 'g');
  }
  f.close();
}

inline void dumpXYZ(const QString& filename, const QList<Eigen::Vector3d>& vs,
                    const QList<int>& ans)
{
  QList<Eigen::Vector3d*> ps;
  for (int i = 0; i < vs.size(); i++)
    ps.append(const_cast<Eigen::Vector3d*>(&vs[i]));
  dumpXYZ(filename, ps, ans);
}

inline void dumpXYZ(const QString& filename, const QList<Atom*>& vs)
{
  QList<Eigen::Vector3d*> ps;
  QList<int> ans;
  for (int i = 0; i < vs.size(); i++) {
    ps.append(const_cast<Eigen::Vector3d*>(vs.at(i)->pos()));
    ans.append(vs.at(i)->atomicNumber());
  }
  dumpXYZ(filename, ps, ans);
}
#endif // DUMP_STRUCTURES

ProtectedCluster* GAPCGenetic::crossover(ProtectedCluster* pc1,
                                         ProtectedCluster* pc2)
{
  INIT_RANDOM_GENERATOR();
  // Create rotation matricies to modify the clusters
  Matrix3d xform1 = createRotationMatrix();
  Matrix3d xform2 = createRotationMatrix();

  // Get lists of atoms and coordinates
  pc1->lock()->lockForRead();
  // Save composition for checks later
  QList<QString> atomSymbols = pc1->getSymbols();
  QList<uint> atomCounts = pc1->getNumberOfAtomsAlpha();
  QList<Atom*> atomList1 = pc1->atoms();
  QList<Vector3d> coordsList1;

#ifdef DUMP_STRUCTURES
  QList<int> ans1;
#endif
  for (int i = 0; i < atomList1.size(); i++) {
    coordsList1.append(*(atomList1.at(i)->pos()));
#ifdef DUMP_STRUCTURES
    ans1.append(atomList1.at(i)->atomicNumber());
#endif
  }
  pc1->lock()->unlock();

  pc2->lock()->lockForRead();
  QList<Atom*> atomList2 = pc2->atoms();
  QList<Vector3d> coordsList2;
#ifdef DUMP_STRUCTURES
  QList<int> ans2;
#endif
  for (int i = 0; i < atomList2.size(); i++) {
    coordsList2.append(*(atomList2.at(i)->pos()));
#ifdef DUMP_STRUCTURES
    ans2.append(atomList2.at(i)->atomicNumber());
#endif
  }
  pc2->lock()->unlock();

#ifdef DUMP_STRUCTURES
  dumpXYZ("crossover01-parent1.xyz", coordsList1, ans1);
  dumpXYZ("crossover01-parent2.xyz", coordsList2, ans2);
#endif

  // Transform atoms -- don't use helper function here since it
  // would need two loops.
  for (int i = 0; i < coordsList1.size(); i++) {
    coordsList1[i] = (coordsList1[i].transpose() * xform1).transpose();
    coordsList2[i] = (coordsList2[i].transpose() * xform2).transpose();
  }

#ifdef DUMP_STRUCTURES
  dumpXYZ("crossover02-xformed1.xyz", coordsList1, ans1);
  dumpXYZ("crossover02-xformed2.xyz", coordsList2, ans2);
#endif

  // Build new cluster
  ProtectedCluster* npc = new ProtectedCluster();
  QWriteLocker npcLocker(npc->lock());

#ifdef DUMP_STRUCTURES
  QList<Eigen::Vector3d> vs1, vs2;
  ans1.clear();
  ans2.clear();
#endif

  // Cut pcs and populate new one.
  for (int i = 0; i < coordsList1.size(); i++) {
    if (coordsList1.at(i)[0] <= 0.0) {
      Atom* newAtom = npc->addAtom();
      newAtom->setAtomicNumber(atomList1.at(i)->atomicNumber());
      newAtom->setPos(coordsList1.at(i));
#ifdef DUMP_STRUCTURES
      ans1.append(atomList1.at(i)->atomicNumber());
      vs1.append(coordsList1.at(i));
#endif
    }
    if (coordsList2.at(i)[0] > 0.0) {
      Atom* newAtom = npc->addAtom();
      newAtom->setAtomicNumber(atomList2.at(i)->atomicNumber());
      newAtom->setPos(coordsList2.at(i));
#ifdef DUMP_STRUCTURES
      ans2.append(atomList2.at(i)->atomicNumber());
      vs2.append(coordsList2.at(i));
#endif
    }
  }

#ifdef DUMP_STRUCTURES
  dumpXYZ("crossover03-cut1.xyz", vs1, ans1);
  dumpXYZ("crossover03-cut2.xyz", vs2, ans2);
  dumpXYZ("crossover04-unchecked.xyz", npc->atoms());
#endif

  // Check composition of npc
  QList<int> deltas;
  QList<QString> nAtomSymbols = npc->getSymbols();
  QList<uint> nAtomCounts = npc->getNumberOfAtomsAlpha();
  // Fill in 0's for any missing atom types in npc
  if (atomSymbols != nAtomSymbols) {
    for (int i = 0; i < atomSymbols.size(); i++) {
      if (i >= nAtomSymbols.size())
        nAtomSymbols.append("");
      if (atomSymbols.at(i) != nAtomSymbols.at(i)) {
        nAtomSymbols.insert(i, atomSymbols.at(i));
        nAtomCounts.insert(i, 0);
      }
    }
  }
  // Get differences --
  // a (+) value in deltas indicates that more atoms are needed in npc
  // a (-) value indicates less are needed.
  for (int i = 0; i < atomCounts.size(); i++)
    deltas.append(atomCounts.at(i) - nAtomCounts.at(i));
  // Correct for differences by inserting atoms from
  // discarded portions of parents or removing random atoms.
  int delta, atomicNumber;
  for (int i = 0; i < deltas.size(); i++) {
    delta = deltas.at(i);
    atomicNumber =
      OpenBabel::etab.GetAtomicNum(atomSymbols.at(i).toStdString().c_str());
    // qDebug() << "Delta = " << delta;
    if (delta == 0)
      continue;
    while (delta < 0) { // qDebug() << "Too many " << atomSymbols.at(i) << "!";
      // Randomly delete atoms from npc;
      // 1 in X chance of each atom being deleted, where
      // X is the total number of that atom type in npc.
      QList<Atom*> atomList = npc->atoms();
      for (int j = 0; j < atomList.size(); j++) {
        if (atomList.at(j)->atomicNumber() == atomicNumber) {
          // atom at j is the type that needs to be deleted.
          if (RANDDOUBLE() < 1.0 / static_cast<double>(nAtomCounts.at(i))) {
            // If the odds are right, delete the atom and break loop to recheck
            // condition.
            npc->removeAtom(atomList.at(
              j)); // removeAtom(Atom*) takes care of deleting pointer.
            delta++;
            break;
          }
        }
      }
    }
    while (delta > 0) { // qDebug() << "Too few " << atomSymbols.at(i) << "!";
      // Randomly add atoms from discarded cuts of parent pcs;
      // 1 in X chance of each atom being added, where
      // X is the total number of atoms of that species in the parent.
      //
      // First, pick the parent. 50/50 chance for each:
      uint parent;
      if (RANDDOUBLE() < 0.5)
        parent = 1;
      else
        parent = 2;
      for (int j = 0; j < coordsList1.size();
           j++) { // size should be the same for both parents
        if (
          // if atom at j is the type that needs to be added,
          ((parent == 1 && atomList1.at(j)->atomicNumber() == atomicNumber) ||
           (parent == 2 && atomList2.at(j)->atomicNumber() == atomicNumber)) &&
          // and atom is in the discarded region of the cut,
          ((parent == 1 && coordsList1.at(j)[0] > 0.0) ||
           (parent == 2 && coordsList2.at(j)[0] <= 0.0)) &&
          // and the odds favor it, add the atom to npc
          (RANDDOUBLE() < 1.0 / static_cast<double>(atomCounts.at(i)))) {
          Atom* newAtom = npc->addAtom();
          newAtom->setAtomicNumber(atomicNumber);
          if (parent == 1)
            newAtom->setPos(coordsList1.at(j));
          else // ( parent == 2)
            newAtom->setPos(coordsList2.at(j));
          delta--;
          break;
        }
      }
    }
  }

#ifdef DUMP_STRUCTURES
  dumpXYZ("crossover05-checked.xyz", npc->atoms());
#endif

  // Done!
  // Expand will center the atoms
  npc->expand(1.2);

#ifdef DUMP_STRUCTURES
  dumpXYZ("crossover06-expanded.xyz", npc->atoms());
#endif

  npc->setStatus(ProtectedCluster::WaitingForOptimization);
  return npc;
}

ProtectedCluster* GAPCGenetic::twist(ProtectedCluster* pc,
                                     double minimumRotation,
                                     double& rotationDeg)
{
  INIT_RANDOM_GENERATOR();
  // Extract data from parent
  pc->lock()->lockForRead();
  QList<Atom*> atoms = pc->atoms();
  vector<Vector3d> coords;
  for (int i = 0; i < atoms.size(); i++)
    coords.push_back(*(atoms.at(i)->pos()));
  pc->lock()->unlock();

#ifdef DUMP_STRUCTURES
  dumpXYZ("twist01-parent.xyz", pc->atoms());
#endif

  // randomly rotate parent coordinates
  coords = rotateCoordinates(coords, createRotationMatrix());

#ifdef DUMP_STRUCTURES
  QList<Vector3d> vs;
  QList<int> ans;
  for (int i = 0; i < coords.size(); i++) {
    vs.append(coords.at(i));
    ans.append(atoms.at(i)->atomicNumber());
  }
  dumpXYZ("twist01-parent-rot.xyz", vs, ans);
#endif

  // Create vector of atoms to twist (positive z coordinate)
  vector<Vector3d> twisters_pos;
  vector<int> twisters_id;
  vector<Vector3d> nontwisters_pos;
  vector<int> nontwisters_id;
  for (int i = 0; i < coords.size(); i++) {
    if (coords.at(i).z() >= 0.0) {
      twisters_pos.push_back(coords.at(i));
      twisters_id.push_back(atoms.at(i)->atomicNumber());
    } else {
      nontwisters_pos.push_back(coords.at(i));
      nontwisters_id.push_back(atoms.at(i)->atomicNumber());
    }
  }

#ifdef DUMP_STRUCTURES
  vs.clear();
  ans.clear();
  for (int i = 0; i < nontwisters_pos.size(); i++) {
    vs.append(nontwisters_pos.at(i));
    ans.append(nontwisters_id.at(i));
  }
  dumpXYZ("twist02-nontwisters.xyz", vs, ans);
  vs.clear();
  ans.clear();
  for (int i = 0; i < twisters_pos.size(); i++) {
    vs.append(twisters_pos.at(i));
    ans.append(twisters_id.at(i));
  }
  dumpXYZ("twist02-twisters.xyz", vs, ans);
#endif

  // Twist the twisters randomly around the z axis
  rotationDeg =
    minimumRotation + (RANDDOUBLE() * (360.0 - (2.0 * minimumRotation)));

  twisters_pos = rotateCoordinates(
    twisters_pos, createRotationMatrix(0, 0, 1, rotationDeg * DEG_TO_RAD));

#ifdef DUMP_STRUCTURES
  vs.clear();
  ans.clear();
  for (int i = 0; i < twisters_pos.size(); i++) {
    vs.append(twisters_pos.at(i));
    ans.append(twisters_id.at(i));
  }
  dumpXYZ("twist03-twistedtwisters.xyz", vs, ans);
#endif

  // Build new cluster
  ProtectedCluster* npc = new ProtectedCluster();
  QWriteLocker npcLocker(npc->lock());

  for (int i = 0; i < twisters_id.size(); i++) {
    Atom* newAtom = npc->addAtom();
    newAtom->setAtomicNumber(twisters_id.at(i));
    newAtom->setPos(twisters_pos.at(i));
  }
  for (int i = 0; i < nontwisters_id.size(); i++) {
    Atom* newAtom = npc->addAtom();
    newAtom->setAtomicNumber(nontwisters_id.at(i));
    newAtom->setPos(nontwisters_pos.at(i));
  }

#ifdef DUMP_STRUCTURES
  dumpXYZ("twist04-child.xyz", npc->atoms());
#endif

  // Done!
  // Expand will center the atoms
  npc->expand(1.2);

#ifdef DUMP_STRUCTURES
  dumpXYZ("twist05-expanded.xyz", npc->atoms());
#endif

  npc->setStatus(ProtectedCluster::WaitingForOptimization);
  return npc;
}

ProtectedCluster* GAPCGenetic::exchange(ProtectedCluster* pc,
                                        unsigned int exchanges)
{
  INIT_RANDOM_GENERATOR();
  // lock parent pc for reading
  QReadLocker locker(pc->lock());

  // Copy info over from parent to new pc
  ProtectedCluster* npc = new ProtectedCluster;
  QWriteLocker npcLocker(npc->lock());
  QList<Atom*> atoms = pc->atoms();
  for (int i = 0; i < atoms.size(); i++) {
    Atom* atom = npc->addAtom();
    atom->setAtomicNumber(atoms.at(i)->atomicNumber());
    atom->setPos(atoms.at(i)->pos());
  }

#ifdef DUMP_STRUCTURES
  dumpXYZ("exchange01-parent.xyz", pc->atoms());
#endif

  // Check that there is more than 1 atom type present.
  // If not, print a warning and return a null pointer;
  if (pc->getSymbols().size() <= 1) {
    qWarning() << "WARNING: "
                  "************************************************************"
                  "*********"
               << endl
               << "WARNING: * Cannot perform exchange with fewer than 2 atomic "
                  "species present. *"
               << endl
               << "WARNING: "
                  "************************************************************"
                  "*********";
    return 0;
  }

  QList<Atom*> natoms = npc->atoms();
  // Swap <exchanges> number of atoms
  for (uint ex = 0; ex < exchanges; ex++) {
    // Generate some indicies
    uint index1 = 0, index2 = 0;
    // Make sure we're swapping different atom types
    while (natoms.at(index1)->atomicNumber() ==
           natoms.at(index2)->atomicNumber()) {
      index1 = index2 = 0;
      while (index1 == index2) {
        index1 = static_cast<uint>(RANDDOUBLE() * natoms.size());
        index2 = static_cast<uint>(RANDDOUBLE() * natoms.size());
      }
    }
    // Swap the atoms
    const Vector3d tmp = *(natoms.at(index1)->pos());
    natoms.at(index1)->setPos(*(natoms.at(index2)->pos()));
    natoms.at(index2)->setPos(tmp);
  }

#ifdef DUMP_STRUCTURES
  dumpXYZ("exchange02-child.xyz", npc->atoms());
#endif

  // Expand will center the atoms
  npc->expand(1.2);

#ifdef DUMP_STRUCTURES
  dumpXYZ("exchange02-expanded.xyz", npc->atoms());
#endif

  return npc;
}

ProtectedCluster* GAPCGenetic::randomWalk(ProtectedCluster* pc,
                                          unsigned int numberAtoms,
                                          double minWalk, double maxWalk)
{
  INIT_RANDOM_GENERATOR();
  // lock parent pc for reading
  QReadLocker locker(pc->lock());

#ifdef DUMP_STRUCTURES
  dumpXYZ("randomWalk01-parent.xyz", pc->atoms());
#endif

  // Copy info over from parent to new pc
  ProtectedCluster* npc = new ProtectedCluster;
  QWriteLocker npcLocker(npc->lock());
  QList<Atom*> atoms = pc->atoms();
  for (int i = 0; i < atoms.size(); i++) {
    Atom* atom = npc->addAtom();
    atom->setAtomicNumber(atoms.at(i)->atomicNumber());
    atom->setPos(atoms.at(i)->pos());
  }
  QList<Atom*> natoms = npc->atoms();

  // Check that the number of requested walkers does not exceed the
  // number of available atoms
  if (numberAtoms > npc->numAtoms()) {
    numberAtoms = npc->numAtoms();
    qWarning() << "GAPCGenetic::randomWalk: "
               << "number of requested walkers exceeds number of "
               << "available atoms. Limiting to " << numberAtoms;
  }

  // Pick walkers
  vector<int> walkers;
  // Select all if all atoms move:
  if (numberAtoms == npc->numAtoms()) {
    for (int i = 0; i < natoms.size(); i++) {
      walkers.push_back(i);
    }
  }
  // otherwise, select randomly
  else {
    for (int i = 0; i < numberAtoms; i++) {
      int index = floor(RANDDOUBLE() * natoms.size());
      // Have we already selected this atom?
      if (find(walkers.begin(), walkers.end(), index) == walkers.end())
        walkers.push_back(index);
      else {
        i--;
        continue;
      }
    }
  }

  // Generate translation vectors
  vector<Vector3d> trans;
  double minWalkSquared = minWalk * minWalk;
  double maxWalkSquared = maxWalk * maxWalk;
  for (int i = 0; i < walkers.size(); i++) {
    double dx = RANDDOUBLE() * maxWalk;
    double dy = RANDDOUBLE() * maxWalk;
    double dz = RANDDOUBLE() * maxWalk;
    double lengthSquared = dx * dx + dy * dy + dz * dz;
    // check length
    if (lengthSquared < minWalkSquared || lengthSquared > maxWalkSquared) {
      i--;
      continue;
    }
    Vector3d t;
    t << dx, dy, dz;
    trans.push_back(t);
  }

  // Walk 'em
  for (int i = 0; i < walkers.size(); i++) {
    int ind = walkers.at(i);
    Atom* atm = natoms.at(ind);
    atm->setPos(*(atm->pos()) + trans.at(i));
  }

#ifdef DUMP_STRUCTURES
  dumpXYZ("randomWalk02-child.xyz", npc->atoms());
#endif

  // Done!
  // Expand will center the atoms
  npc->expand(1.2);

#ifdef DUMP_STRUCTURES
  dumpXYZ("randomWalk03-expanded.xyz", npc->atoms());
#endif

  npc->setStatus(ProtectedCluster::WaitingForOptimization);
  return npc;
}

ProtectedCluster* GAPCGenetic::anisotropicExpansion(ProtectedCluster* pc,
                                                    double amp)
{
  INIT_RANDOM_GENERATOR();

  // Extract data from parent
  pc->lock()->lockForWrite();
  pc->centerAtoms();
  pc->lock()->unlock();
  pc->lock()->lockForRead();
  QList<Atom*> atoms = pc->atoms();
  vector<Vector3d> coords;
  for (int i = 0; i < atoms.size(); i++)
    coords.push_back(*(atoms.at(i)->pos()));
  pc->lock()->unlock();

#ifdef DUMP_STRUCTURES
  dumpXYZ("aniso01-parent.xyz", pc->atoms());
#endif

  // randomly rotate parent coordinates
  coords = rotateCoordinates(coords, createRotationMatrix());

#ifdef DUMP_STRUCTURES
  QList<Eigen::Vector3d> vs;
  for (int i = 0; i < coords.size(); i++)
    vs.append(coords.at(i));
  QList<int> ans;
  for (int i = 0; i < atoms.size(); i++)
    ans.append(atoms.at(i)->atomicNumber());
  dumpXYZ("aniso02-rotated.xyz", vs, ans);
#endif

  // Perform expansion
  double rho, phi, theta, x, y, z;
  Eigen::Vector3d npos, *pos;
  for (int i = 0; i < coords.size(); i++) {
    // Convert cartestian coords to spherical coords
    pos = &(coords[i]);
    x = pos->x();
    y = pos->y();
    z = pos->z();
    rho = sqrt(x * x + y * y + z * z);
    phi = acos(z / rho);
    theta = asin(y / sqrt(x * x + y * y));
    if (x < 0)
      theta = M_PI - theta;
    // Expand
    double factor = (cos(2 * phi) + 1) / 2.0;
    // cube factor
    rho *= 1 + amp * factor * factor * factor * factor;
    // Back to cartesian
    pos->x() = rho * sin(phi) * cos(theta);
    pos->y() = rho * sin(phi) * sin(theta);
    pos->z() = rho * cos(phi);
  }

  // Build new cluster
  ProtectedCluster* npc = new ProtectedCluster();
  QWriteLocker npcLocker(npc->lock());

  for (int i = 0; i < atoms.size(); i++) {
    Atom* newAtom = npc->addAtom();
    newAtom->setAtomicNumber(atoms.at(i)->atomicNumber());
    newAtom->setPos(coords.at(i));
  }

#ifdef DUMP_STRUCTURES
  dumpXYZ("aniso03-child.xyz", npc->atoms());
#endif

  // Done!
  npc->setStatus(ProtectedCluster::WaitingForOptimization);
  return npc;
}
}
