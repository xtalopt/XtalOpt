/**********************************************************************
  SubMoleculeSource - Generate/Manage conformers for a molecular unit

  Copyright (C) 2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifndef SUBMOLECULESOURCE_H
#define SUBMOLECULESOURCE_H

#define SUBMOLECULESOURCE_MAXCONFORMERS 20
#define SUBMOLECULESOURCE_NUMGEOSTEPS 2500

#include <globalsearch/structure.h>

#include <QtCore/QString>

class QSettings;
class QMutex;

class SubMoleculeSourceTest;

namespace OpenBabel {
  class OBForceField;
  class OBMol;
}

namespace XtalOpt {
  class MolecularXtal;
  class SubMolecule;

  class SubMoleculeSource : public GlobalSearch::Structure
  {
    Q_OBJECT

  public:
    SubMoleculeSource(QObject *parent = 0);
    SubMoleculeSource(Avogadro::Molecule *mol, QObject *parent = 0);
    SubMoleculeSource(OpenBabel::OBMol *mol, QObject *parent = 0);
    virtual ~SubMoleculeSource();

    unsigned int maxConformers() { return m_maxConformers;}
    unsigned int numGeoSteps() { return m_numGeoSteps;}

    // Returns a submolecule built from conformer i
    SubMolecule * getSubMolecule(int index = 0);
    // Returns a new random representation of the submolecule
    SubMolecule * getRandomSubMolecule();

    bool readFromSettings(QSettings *settings);
    bool writeToSettings(QSettings *settings) const;

    unsigned long sourceId() const {return m_sourceId;}
    QString name() {return m_name;}

    friend class ::SubMoleculeSourceTest;

  public slots:
    void set(Avogadro::Molecule *mol);
    void set(OpenBabel::OBMol *mol);
    void setMaxConformers(unsigned int i) {m_maxConformers = i;}
    void setNumGeoSteps(unsigned int i) {m_numGeoSteps = i;}
    void setSourceId(unsigned long i) {m_sourceId = i;}
    void setName(const QString &s) {m_name = s;}
    unsigned int findAndSetConformers();

  signals:
    void conformerGenerated(int current, int total);

  protected:
    bool setupForcefield();
    bool initializeMMFF94s();
    bool initializeUFF();
    void sortConformersByEnergy();
    unsigned int weightedRandomConformerIndex();
    unsigned int m_maxConformers;
    unsigned int m_numGeoSteps;

    OpenBabel::OBForceField *m_ff;
    QMutex *m_ffMutex;
    unsigned long m_sourceId;
    QString m_name;

  };

} // end namespace XtalOpt

#endif
