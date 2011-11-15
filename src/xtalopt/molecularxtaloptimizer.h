/****************************************************************************
  MolecularXtalOptimizer: Forcefield geometry optimizations of MolecularXtals
  Copyright (C) 2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the Free
  Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
  more details.
 ****************************************************************************/

#ifndef MOLECULARXTALOPTIMIZER_H
#define MOLECULARXTALOPTIMIZER_H

#include <QObject>

class QMutex;
class ostream;

namespace XtalOpt
{
class MolecularXtal;
class MolecularXtalOptimizerPrivate;
class MolecularXtalOptimizer : public QObject
{
  Q_OBJECT
public:
  explicit MolecularXtalOptimizer(QObject *parent = NULL,
                                  QMutex *setupMutex = NULL);
  virtual ~MolecularXtalOptimizer();

public Q_SLOTS:
  // Enable/disable debugging
  void setDebug(bool b);

  // Setup calcs
  void setMXtal(MolecularXtal *mxtal);
  void setSuperCellUpdateInterval(int interval);
  void setNumberOfGeometrySteps(int steps);
  void setEnergyConvergence(double conv);

  // Cutoffs
  void setVDWCutoff(double cutoff);
  void setElectrostaticCutoff(double cutoff);
   // -1 only updates with cell (default):
  void setCutoffUpdateInterval(int interval);

public:
  // Whether or not debugging is enabled
  bool debug() const;

  // Retrieve calc settings
  MolecularXtal * mxtal() const;
  int superCellUpdateInterval() const;
  int numberOfGeometrySteps() const;
  int energyConvergence() const;

  // Retrieve cutoff info
  double vdwCutoff() const;
  double electrostaticCutoff() const;
  int cutoffUpdateInterval() const;

  // Control calculations
  bool setup();
  void run();
  bool waitForFinished(int ms_timeout = -1) const; // ret false if timeout
  void abort();

  // Determine calculation outcome
  bool isRunning() const;
  bool isConverged() const;
  bool reachedStepLimit() const;
  bool isAborted() const;

public Q_SLOTS:
  // Update the pointer set by setMxtal
  void updateMXtalCoords() const;
  void updateMXtalEnergy() const;

  // Disconnect the MolecularXtal after optimization is complete and the
  // results have been copied with the updateMXtal* methods. This must be
  // called if setMXtal is called. This function also must be called under a
  // WriteLock on the MolecularXtal's rwLock.
  void releaseMXtal();

public Q_SLOTS:
  // For debugging
  void printSelf() const;

signals:
  void startingOptimization();
  void finishedOptimization();

  void finishedGeometryStep(int step);

protected:
  MolecularXtalOptimizerPrivate * const d_ptr;

private:
  Q_DECLARE_PRIVATE(MolecularXtalOptimizer);
};

}

#endif // MOLECULARXTALOPTIMIZER_H
