/****************************************************************************
  MolecularXtalMutator -- Perform smart mutations on MolecularXtals

  Copyright (C) 2012 by David C. Lonie

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the Free
  Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
  more details.
 ****************************************************************************/

#ifndef MOLECULARXTALMUTATOR_H
#define MOLECULARXTALMUTATOR_H

#include <QtCore/QObject>

class QMutex;

namespace XtalOpt
{
class MolecularXtal;
class MolecularXtalMutatorPrivate;

class MolecularXtalMutator : public QObject
{
  Q_OBJECT
public:
  explicit MolecularXtalMutator(const MolecularXtal *mxtal,
                                QMutex *setupMutex = NULL,
                                QObject *parent = NULL);
  virtual ~MolecularXtalMutator();

public Q_SLOTS:
  // Enable/disable debugging
  void setDebug(bool b);

  ///////////////////////////////
  // Set parameters for mutations
  //   Ignore probabilities and always pick best mutations
  void setCreateBest(bool b);
  //   Reset to a supercell from the best atoms before mutation
  void setStartWithSuperCell(bool b);
  void setMaximumNumberOfCandidates(unsigned int num);
  void setNumberOfStrains(unsigned int strains);
  void setStrainSigmaRange(double min, double max);
  void setNumMovers(unsigned int numMovers);
  void setNumDisplacements(unsigned int numDisplacements);
  void setRotationResolution(double radians);
  void setNumberOfVolumeSamples(unsigned int numSamples);
  void setMinimumVolumeFraction(double frac); // Default: 0.75
  void setMaximumVolumeFraction(double frac); // Default: 1.25

  // Start the mutation process in a background thread. True if successful
  // and offspring created, false otherwise (e.g. aborted).
  bool mutate();

  // Abort the mutation process
  void abort();

public:
  // Retrieve mutation parameters
  bool createBest() const;
  bool startWithSuperCell() const;
  unsigned int maximumNumberOfCandidates() const;
  unsigned int numberOfStrains() const;
  void strainSigmaRange(double range[2]) const;
  unsigned int numberOfMovers() const;
  unsigned int numberOfDisplacements() const;
  double rotationResolution() const; // in radian
  double numberOfVolumeSamples() const;
  double minimumVolumeFraction() const;
  double maximumVolumeFraction() const;

  // Block until mutation is complete. Return false if timeout, -1 = no timeout
  bool waitForFinished(int ms_timeout = -1) const;

  // Check status
  bool isRunning() const;
  bool isAborted() const;

  // Retrieve structures
  const MolecularXtal *getParentMXtal() const;
  // Creates an unowned copy of the offspring (you must delete it).
  // Returns NULL if mutation has not completed
  MolecularXtal * getOffspring() const;

Q_SIGNALS:
  void startingMutation();
  void progressUpdate(int progress); // progress is [0,100]
  void finishedMutation();

protected:
  MolecularXtalMutatorPrivate * const d_ptr;

private:
  Q_DECLARE_PRIVATE(MolecularXtalMutator);
};

}

#endif // MOLECULARXTALMUTATOR_H
