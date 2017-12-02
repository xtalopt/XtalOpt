/**********************************************************************
  Cluster - Implementation of an atomic cluster

  Copyright (C) 2010-2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <gapc/structures/cluster.h>

#include <globalsearch/macros.h>

#include <avogadro/neighborlist.h>

#include <Eigen/Array>

#include <algorithm>
#include <deque>
#include <vector>

using namespace Avogadro;
using namespace std;

namespace GAPC {

Cluster::Cluster(QObject* parent) : Structure(parent)
{
}

Cluster::~Cluster()
{
}

void Cluster::constructRandomCluster(
  const QHash<unsigned int, unsigned int>& comp, float minIAD, float maxIAD)
{
  INIT_RANDOM_GENERATOR();
  // Get atomic numbers
  QList<unsigned int> atomicnums = comp.keys();
  unsigned int totalSpecies = atomicnums.size();

  // Get total number of atoms
  unsigned int totalAtoms = 0;
  for (int i = 0; i < totalSpecies; i++) {
    totalAtoms += comp.value(atomicnums.at(i));
  }

  // Use this queue to determine the order to add the atoms. Value
  // is atomic number
  deque<unsigned int> q;

  // Populate queue
  // - Fill queue
  for (int i = 0; i < totalSpecies; i++) {
    unsigned int atomicnum = atomicnums.at(i);
    for (int j = 0; j < comp[atomicnum]; j++) {
      q.push_back(atomicnum);
    }
  }
  // - Randomize
  random_shuffle(q.begin(), q.end());

  // Populate cluster
  clear();
  Eigen::Vector3d tmpvec;
  QList<Atom*> neighbors;
  QList<double> distances;
  while (!q.empty()) {
    // Center the molecule at the origin
    centerAtoms();
    // Upper limit for new position distance
    double max = radius() + maxIAD;
    // temp coordinates
    double x, y, z;
    // Set first atom to origin
    if (numAtoms() == 0) {
      x = y = z = 0.0;
    }
    // Set second atom randomly between minIAD and maxIAD from origin
    else if (numAtoms() == 1) {
      tmpvec.setRandom();
      tmpvec.normalize();
      tmpvec = tmpvec * (maxIAD - minIAD) * RANDDOUBLE() +
               (Eigen::Vector3d::Ones() * minIAD);
      x = tmpvec.x();
      y = tmpvec.y();
      z = tmpvec.z();
    }
    // Randomly generate other atoms, ensuring that they lie between
    // minIAD and maxIAD from at least two other atoms and no more
    // than minIAD from any atom
    else {
      forever
      {
        // Randomly generate coordinates
        tmpvec.setRandom();
        tmpvec.normalize();
        tmpvec = tmpvec * max * RANDDOUBLE();
        x = tmpvec.x();
        y = tmpvec.y();
        z = tmpvec.z();
        neighbors = getNeighbors(x, y, z, maxIAD, &distances);

        Q_ASSERT(neighbors.size() == distances.size());

        // If not neighbors, continue
        if (neighbors.size() < 2) {
          continue;
        }

        unsigned int found = 0;
        bool invalid = false;
        for (QList<double>::const_iterator dit = distances.constBegin(),
                                           dit_end = distances.constEnd();
             dit != dit_end; ++dit) {
          // check that the distance is greater than minIAD from the new point
          if (*dit < minIAD) {
            invalid = true;
            break;
          }
          ++found;
        }

        // If the conditions aren't met, generate a new set of coordinates
        if (invalid || found < 2) {
          continue;
        }
        // Otherwise add the atom.
        break;
      }
    }
    Atom* atm = addAtom();
    Eigen::Vector3d pos(x, y, z);
    atm->setPos(pos);
    atm->setAtomicNumber(q.front());
    q.pop_front();
  }

  resetEnergy();
  resetEnthalpy();
  emit moleculeChanged();
}

void Cluster::centerAtoms()
{
  translate(-center());
}

QHash<QString, QVariant> Cluster::getFingerprint() const
{
  QHash<QString, QVariant> fp = Structure::getFingerprint();
  fp.insert("IADFreq", QVariant(m_histogramFreq));
  fp.insert("IADDist", QVariant(m_histogramDist));
  return fp;
}

Eigen::Vector3f ev3dToev3f(const Eigen::Vector3d* v)
{
  return Eigen::Vector3f(v->x(), v->y(), v->z());
}

inline void QListUniqueAppend(QList<Atom*>* perm, const QList<Atom*>* tmp)
{
  for (int i = 0; i < tmp->size(); i++) {
    if (!perm->contains(tmp->at(i))) {
      perm->append(tmp->at(i));
    }
  }
}

bool Cluster::checkForExplosion(double rcut) const
{
  NeighborList nl(const_cast<Cluster*>(this), rcut);
  QList<Atom *> found, tmp;

  // The found list is treated as unique list, using the
  // QListUniqueAppend function above. We'll start at the farthest
  // atom from the molecule's center, then iteratively add all
  // neighbors within rcut. This is repeated until we have added all
  // atoms that we can using this method of traversal. Then we
  // compare the size of the found tracker to numAtoms() to see if
  // all atoms were found using this method.

  const Atom* start = farthestAtom();
  Eigen::Vector3f fpos = ev3dToev3f(start->pos());
  tmp = nl.nbrs(&fpos);
  QListUniqueAppend(&found, &tmp);

  for (int i = 0; i < found.size(); i++) {
    Eigen::Vector3f fpos = ev3dToev3f(found.at(i)->pos());
    tmp = nl.nbrs(&fpos);
    QListUniqueAppend(&found, &tmp);
  }

  if (found.size() != numAtoms()) {
    return false;
  }
  return true;
}

void Cluster::expand(double factor)
{
  // Center atom
  centerAtoms();

  // Perform expansion on all atoms
  double rho, phi, theta, x, y, z;
  Atom* atom;
  const Eigen::Vector3d* pos;
  Eigen::Vector3d npos;
  for (int i = 0; i < numAtoms(); i++) {
    // Convert cartestian coords to spherical coords
    atom = atoms().at(i);
    pos = atom->pos();
    x = pos->x();
    y = pos->y();
    z = pos->z();
    rho = sqrt(x * x + y * y + z * z);
    phi = acos(z / rho);
    theta = asin(y / sqrt(x * x + y * y));
    if (x < 0)
      theta = M_PI - theta;
    // Expand
    rho *= factor;
    // Back to cartesian
    npos.x() = rho * sin(phi) * cos(theta);
    npos.y() = rho * sin(phi) * sin(theta);
    npos.z() = rho * cos(phi);
    atom->setPos(&npos);
  }
  emit update();
}

} // end namespace GAPC
