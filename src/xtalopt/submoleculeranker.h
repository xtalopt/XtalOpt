/****************************************************************************
  SubMoleculeRanker: Evaluate and rank SubMolecules in a MolecularXtal by
  forcefield energies

  Copyright (C) 2012 by David C. Lonie

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the Free
  Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
  more details.
 ****************************************************************************/

#ifndef SUBMOLECULERANKER_H
#define SUBMOLECULERANKER_H

#include <QtCore/QObject>

#include <QtCore/QVector> // needed for default arg

class QMutex;

namespace XtalOpt
{
class MolecularXtal;
class SubMolecule;
class SubMoleculeRankerPrivate;

class SubMoleculeRanker : public QObject
{
  Q_OBJECT
public:
  explicit SubMoleculeRanker(QObject *parent = NULL,
                             QMutex *setupMutex = NULL);
  virtual ~SubMoleculeRanker();

public Q_SLOTS:
  // Enable/disable debugging
  void setDebug(bool b);

  // Setup calcs
  bool setMXtal(MolecularXtal *mxtal);

  // Notify that geometry of mxtal has changed
  void updateGeometry(); // for entire mxtal
  void updateGeometry(const SubMolecule *subMol); // or just one SubMolecule

  // Cutoffs
  void setVDWCutoff(double cutoff);
  void setElectrostaticCutoff(double cutoff);

public:
  // Whether or not debugging is enabled
  bool debug() const;

  // Retrieve calc settings
  MolecularXtal * mxtal() const;

  // Retrieve cutoff info
  double vdwCutoff() const;
  double electrostaticCutoff() const;

  // Calculate energies for all submolecules by submol index
  QVector<double> evaluate();
  QVector<double> evaluateInter(); // Intermolecular (bonded) terms only
  QVector<double> evaluateIntra(); // Intramolecular (non-bonded) terms only

  // Calculate energy for given SubMolecule
  double evaluate(const SubMolecule *subMol);
  double evaluateInter(const SubMolecule *subMol);
  double evaluateIntra(const SubMolecule *subMol);

  // Calculate total energy of system (intramolecular terms for original cell,
  // intermolecular terms between original cell and all atoms + images.
  // If an argument is provided, only the energy terms for the submolecules
  // indicated will be calculated. The "ignore" flag (default = true)
  // specifies whether the indicated submolecules are meant to be ignored in
  // the calcs (true) or the only ones considered (false)
  double evaluateTotalEnergy(
      const QVector<SubMolecule*> &submols = QVector<SubMolecule*>(),
      bool ignore = true);
  double evaluateTotalEnergyInter(
      const QVector<SubMolecule*> &submols = QVector<SubMolecule*>(),
      bool ignore = true);
  double evaluateTotalEnergyIntra(
      const QVector<SubMolecule*> &submols = QVector<SubMolecule*>(),
      bool ignore = true);

protected:
  SubMoleculeRankerPrivate * const d_ptr;

private:
  Q_DECLARE_PRIVATE(SubMoleculeRanker);
};

}

#endif // SUBMOLECULERANKER_H
