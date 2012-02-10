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

#ifndef MOLECULARXTAL_H
#define MOLECULARXTAL_H

#include <xtalopt/structures/xtal.h>

namespace XtalOpt {
  class SubMolecule;
  class MolecularXtalOptimizer;
  class MolecularXtalOptimizerPrivate;
  class MolecularXtal : public Xtal
  {
    Q_OBJECT

  public:
    MolecularXtal(QObject *parent = 0);
    MolecularXtal(double A, double B, double C,
                  double Alpha, double Beta, double Gamma,
                  QObject *parent = 0);
    MolecularXtal(const MolecularXtal &other);

    virtual ~MolecularXtal();

    MolecularXtal& operator=(const MolecularXtal &other)
    {
      this->Xtal::operator=(other);
      return *this;
    }
    GlobalSearch::Structure& copyStructure(
        const GlobalSearch::Structure &other);

    bool operator==(const MolecularXtal &other) const;
    bool operator!=(const MolecularXtal &other) const
    {
      return !operator==(other);
    }
    // Tolerances in angstrom and degree. This comparison does not consider
    // hydrogen atoms.
    bool compareCoordinates(const MolecularXtal &other, const double tol = 0.1,
                            const double angleTol = 2.0) const;

    int numSubMolecules() const;
    QList<SubMolecule*> subMolecules() const;
    SubMolecule * subMolecule(int index);
    const SubMolecule * subMolecule(int index) const;

    // Test that submolecules haven't exploded (e.g. bond lengths aren't too
    // long). Return true if all bonds are ok.
    bool verifySubMolecules() const;

    //! For each bond, check if all non-bonded atoms are at least @a
    //! minDistance from the bond. This essentially creates a cylinder of
    //! radius @a minDistance around the bond that atoms are not allowed to be
    //! in.
    bool checkAtomToBondDistances(double minDistance);

    int getPreOptProgress() const
    {
      return (this->m_status == Preoptimizing && m_preOptStepCount != 0)
          ? (100 * m_preOptStep) / m_preOptStepCount : -1;
    }
    MolecularXtalOptimizer *getPreoptimizer() {return m_mxtalOpt;}
    const MolecularXtalOptimizer *getPreoptimizer() const {return m_mxtalOpt;}

    bool needsPreoptimization() const {return m_needsPreOpt;}
    bool isPreoptimizing() const;

    // no set function -- m_mxtalOpt is set automatically when the calcs are
    // setup in MolecularXtalOptimizer
    MolecularXtalOptimizer * getOptimizer() const {return m_mxtalOpt;}

    friend class MolecularXtalOptimizer;
    friend class MolecularXtalOptimizerPrivate;

  public slots:
    // Reimplementations act on molecule centers rather than atomic positions
    virtual void setVolume(double Volume);
    virtual void rescaleCell(double a, double b, double c,
                             double alpha, double beta, double gamma);


    // Spacegroup -- reimplementation skips hydrogens
    virtual void findSpaceGroup(double prec = 0.05);

    void addSubMolecule(SubMolecule *sub);
    void removeSubMolecule(SubMolecule *sub);
    void replaceSubMolecule(int i, SubMolecule *newSub);

    virtual void readSettings(const QString &filename) {
      this->Xtal::readSettings(filename);
      // this must be called separately after the structure has loaded
      //this->readMolecularXtalSettings(filename);
    }
    virtual void writeSettings(const QString &filename) {
      this->Xtal::writeSettings(filename);
      this->writeMolecularXtalSettings(filename);
    }

    void readMolecularXtalSettings(const QString &filename);
    void writeMolecularXtalSettings(const QString &filename) const;

    void setPreOptStepCount(int i) {m_preOptStepCount = i;}
    void setPreOptStep(int i) {m_preOptStep = i;}
    void setNeedsPreoptimization(bool b) {m_needsPreOpt = b;}
    void abortPreoptimization() const;

    void makeCoherent();

  protected:
    QList<SubMolecule*> m_subMolecules;
    int m_preOptStepCount;
    int m_preOptStep;
    bool m_needsPreOpt;
    MolecularXtalOptimizer *m_mxtalOpt;

  };

} // end namespace XtalOpt

#endif
