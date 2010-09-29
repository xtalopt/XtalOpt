/**********************************************************************
  Cluster - Implementation of an atomic cluster

  Copyright (C) 2010 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include <gapc/structures/cluster.h>

#include <globalsearch/macros.h>

#include <algorithm>
#include <vector>
#include <deque>

using namespace std;

namespace GAPC {

  Cluster::Cluster(QObject *parent) :
    Structure(parent)
  {
  }

  Cluster::~Cluster()
  {
  }

  bool Cluster::compareNearestNeighborDistributions(const QList<double> &d,
                                                    const QList<double> &f1,
                                                    const QList<double> &f2,
                                                    double decay,
                                                    double smear,
                                                    double *error)
  {
    // Check that smearing is possible
    if (smear != 0 && d.size() <= 1) {
      qWarning() << "Cluster::compareNNDist: Cannot smear with 1 or fewer points.";
      return false;
    }
    // Check sizes
    if (d.size() != f1.size() || f1.size() != f2.size()) {
      qWarning() << "Cluster::compareNNDist: Vectors are not the same size.";
      return false;
    }

    // Perform a boxcar smoothing over range set by "smear"
    // First determine step size of d, then convert smear to index units
    double stepSize = fabs(d.at(1) - d.at(0));
    int boxSize = ceil(smear/stepSize);
    if (boxSize > d.size()) {
      qWarning() << "Cluster::compareNNDist: Smear length is greater then d vector range.";
      return false;
    }
    // Smear
    vector<double> f1s, f2s, ds; // smeared vectors
    double f1t, f2t, dt; // temporary variables
    for (int i = 0; i < d.size() - boxSize; i++) {
      f1t = f2t = dt = 0;
      for (int j = 0; j < boxSize; j++) {
        f1t += f1.at(i+j);
        f2t += f2.at(i+j);
      }
      f1s.push_back(f1t / double(boxSize));
      f2s.push_back(f2t / double(boxSize));
      ds.push_back(dt / double(boxSize));
    }

    // Calculate diff vector
    vector<double> diff;
    for (int i = 0; i < ds.size(); i++) {
      diff.push_back(fabs(f1s.at(i) - f2s.at(i)));
    }

    // Calculate decay function: Standard exponential decay with a
    // halflife of decay. If decay==0, no decay.
    double decayFactor = 0;
    // ln(2) / decay:
    if (decay != 0) {
      decayFactor = 0.69314718055994530941723 / decay;
    }

    // Calculate error:
    (*error) = 0;
    for (int i = 0; i < ds.size(); i++) {
      (*error) += exp(-decayFactor * ds.at(i)) * diff.at(i);
    }

    return true;
  }


  void Cluster::constructRandomCluster(const QHash<unsigned int, unsigned int> &comp,
                                       float minIAD,
                                       float maxIAD)
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
    while (!q.empty()) {
      // Center the molecule at the origin
      centerAtoms();
      // Upper limit for new position distance
      double max = radius() + maxIAD;
      double x, y, z;
      double shortest;
      // Set first atom to origin
      if (numAtoms() == 0) {
        x = y = z = 0.0;
      }
      // Randomly generate other atoms
      else {
        do {
          // Randomly generate coordinates
          x = RANDDOUBLE() * max;
          y = RANDDOUBLE() * max;
          z = RANDDOUBLE() * max;
          getNearestNeighborDistance(x, y, z, shortest);
        } while (shortest > maxIAD || shortest < minIAD);
      }
      Atom *atm = addAtom();
      Eigen::Vector3d pos (x, y, z);
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
    QList<double> dist, freq;
    QList<QVariant> vdist, vfreq;
    getNearestNeighborHistogram(dist, freq, 0, 10, 0.01);
    for (int i = 0; i < dist.size(); i++) {
      vdist.append(dist.at(i));
      vfreq.append(freq.at(i));
    }
    fp.insert("IADFreq", QVariant(vfreq));
    fp.insert("IADDist", QVariant(vdist));
    return fp;
  }


} // end namespace GAPC

//#include "scene.moc"
