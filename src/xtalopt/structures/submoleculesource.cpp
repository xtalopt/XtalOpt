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

#include <xtalopt/structures/submoleculesource.h>

#include <xtalopt/structures/molecularxtal.h>
#include <xtalopt/structures/submolecule.h>

#include <globalsearch/macros.h>

#include <avogadro/atom.h>
#include <avogadro/bond.h>

#include <openbabel/atom.h>
#include <openbabel/bond.h>
#include <openbabel/obconversion.h>
#include <openbabel/forcefield.h>
#include <openbabel/mol.h>
#include <openbabel/obiter.h>

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QSettings>
#include <QtCore/QString>
#include <QtCore/QVariant>

#include <Eigen/Array>

#include <limits>
#include <sstream>
#include <vector>

using namespace Avogadro;
using OpenBabel::OBMolAtomIter;
using OpenBabel::OBMolBondIter;

namespace XtalOpt {

  SubMoleculeSource::SubMoleculeSource(QObject *parent)
    : GlobalSearch::Structure(parent),
      m_maxConformers(SUBMOLECULESOURCE_MAXCONFORMERS),
      m_numGeoSteps(SUBMOLECULESOURCE_NUMGEOSTEPS),
      m_ff(NULL),
      m_ffMutex(new QMutex),
      m_sourceId(std::numeric_limits<unsigned long>::max())
  {
  }

  SubMoleculeSource::SubMoleculeSource(Avogadro::Molecule *mol,
                                       QObject *parent)
    : GlobalSearch::Structure(parent),
      m_maxConformers(SUBMOLECULESOURCE_MAXCONFORMERS),
      m_numGeoSteps(SUBMOLECULESOURCE_NUMGEOSTEPS),
      m_ff(NULL),
      m_ffMutex(new QMutex),
      m_sourceId(std::numeric_limits<unsigned long>::max())
  {
    this->set(mol);
  }

  SubMoleculeSource::SubMoleculeSource(OpenBabel::OBMol *mol,
                                       QObject *parent)
    : GlobalSearch::Structure(parent),
      m_maxConformers(SUBMOLECULESOURCE_MAXCONFORMERS),
      m_numGeoSteps(SUBMOLECULESOURCE_NUMGEOSTEPS),
      m_ff(NULL),
      m_ffMutex(new QMutex),
      m_sourceId(std::numeric_limits<unsigned long>::max())
  {
    this->set(mol);
  }

  SubMoleculeSource::~SubMoleculeSource()
  {
    delete m_ff;
    m_ff = NULL;

    delete m_ffMutex;
    m_ffMutex = NULL;
  }

  bool SubMoleculeSource::readFromSettings(QSettings *settings)
  {
    this->clear();
    Q_ASSERT_X(settings, Q_FUNC_INFO, "QSettings object is NULL!");

    // Metadata
    this->m_sourceId = static_cast<unsigned long>
        (settings->value("id", static_cast<unsigned long long>
                         (std::numeric_limits<unsigned long>::max()))
         .toULongLong());
    this->m_name = settings->value("name", "").toString();

    // Read atom types
    int atomCount = settings->beginReadArray("atoms");
    for (int i = 0; i < atomCount; ++i) {
      settings->setArrayIndex(i);
      int an = settings->value("atomicNum", -1).toInt();
      if (an < 0) {
        qWarning() << "Invalid atomic number" << an << "for atom at index" << i;
        return false;
      }

      Atom *a = this->addAtom();
      a->setAtomicNumber(an);
    }
    settings->endArray(); // "atoms"

    // Read bond info
    int bondCount = settings->beginReadArray("bonds");
    for (int i = 0; i < bondCount; ++i) {
      settings->setArrayIndex(i);
      int begIndex = settings->value("beginInd", -1).toInt();
      int endIndex = settings->value("endInd", -1).toInt();
      int order    = settings->value("order", -1).toInt();
      if (begIndex < 0 || begIndex >= atomCount ||
          endIndex < 0 || endIndex >= atomCount ||
          order < 0) {
        qWarning() << "Invalid bond specification: beg,end index:" << begIndex
                   << endIndex << "order:" << order
                   << "(numAtoms:" << atomCount << ")";
        return false;
      }

      Bond *b = this->addBond();
      b->setBegin(m_atomList[begIndex]);
      b->setEnd(m_atomList[endIndex]);
      b->setOrder(order);
    }
    settings->endArray(); // "bonds"

    // Create conformer container
    std::vector<std::vector<Eigen::Vector3d>*> confs;
    std::vector<double> nrgs;

    // Get conformers array
    int lenConformerArr = settings->beginReadArray("conformers");
    confs.reserve(lenConformerArr);
    nrgs.reserve(lenConformerArr);
    for (int i = 0; i < lenConformerArr; ++i) {
      settings->setArrayIndex(i);

      // Read "energy" value
      double nrg = settings->value("energy", 0.0).toDouble();

      // Read atomic positions
      int lenPosArr = settings->beginReadArray("positions");
      if (lenPosArr != this->numAtoms()) {
        qWarning() << "Skipping conformer set" << i << "for SubMoleculeSource"
                   << m_sourceId << ". NumAtoms =" << this->numAtoms()
                   << "numPositions =" << lenPosArr;
        continue;
      }
      std::vector<Eigen::Vector3d> * conf =
          new std::vector<Eigen::Vector3d> (lenPosArr);
      for (int j = 0; j < lenPosArr; ++j) {
        settings->setArrayIndex(j);
        Eigen::Vector3d &pos = (*conf)[j];
        pos.x() = settings->value("x", 0.0).toDouble();
        pos.y() = settings->value("y", 0.0).toDouble();
        pos.z() = settings->value("z", 0.0).toDouble();
      }
      confs.push_back(conf);
      nrgs.push_back(nrg);
      settings->endArray();
    }
    settings->endArray();

    if (!this->setAllConformers(confs)) {
      qWarning() << "Error setting conformers for SubMoleculeSource"
                 << m_sourceId;
      return false;
    }
    this->setEnergies(nrgs);

    return true;
  }

  bool SubMoleculeSource::writeToSettings(QSettings *settings) const
  {
    Q_ASSERT_X(settings, Q_FUNC_INFO, "QSettings object is NULL!");

    // Metadata
    settings->setValue("id", static_cast<unsigned long long>(this->m_sourceId));
    settings->setValue("name", this->m_name);

    // Write atom types
    settings->beginWriteArray("atoms", m_atomList.size());
    for (int i = 0; i < m_atomList.size(); ++i) {
      settings->setArrayIndex(i);
      settings->setValue("atomicNum", m_atomList[i]->atomicNumber());
    }
    settings->endArray(); // "atoms"

    // Write bond info
    settings->beginWriteArray("bonds", m_bondList.size());
    for (int i = 0; i < m_bondList.size(); ++i) {
      settings->setArrayIndex(i);
      const Bond *b = m_bondList[i];
      settings->setValue("beginInd", static_cast<int>(b->beginAtom()->index()));
      settings->setValue("endInd", static_cast<int>(b->endAtom()->index()));
      settings->setValue("order", static_cast<int>(b->order()));
    }
    settings->endArray(); // "bonds"

    // Each conformer is written as energy and xyz coord:
    const std::vector<std::vector<Eigen::Vector3d>*>
        &confs = this->conformers();
    const std::vector<double> &nrgs = this->energies();

    settings->beginWriteArray("conformers", confs.size());
    for (int ind = 0; ind < confs.size(); ++ind) {
      settings->setArrayIndex(ind);

      const double nrg = nrgs[ind];
      settings->setValue("energy", nrg);

      const std::vector<Eigen::Vector3d> *conf = confs[ind];
      settings->beginWriteArray("positions", conf->size());
      int coordInd = 0;
      foreach (const Atom *a, this->m_atomList) {
        settings->setArrayIndex(coordInd++);

        const Eigen::Vector3d &vec = (*conf)[a->id()];
        settings->setValue("x", vec.x());
        settings->setValue("y", vec.y());
        settings->setValue("z", vec.z());
      }
      settings->endArray(); // "positions"
    }
    settings->endArray(); // "conformers"

    return true;
  }

  SubMolecule * SubMoleculeSource::getSubMolecule(int index)
  {
    if (index < 0 || index > this->numConformers() - 1) {
      qWarning() << "Requested submolecule conformer that doesn't exist:"
                 << index << "max:" << this->numConformers() - 1;
      return NULL;
    }

    std::vector<Eigen::Vector3d> *conf = this->conformer(index);

    // Create submolecule
    SubMolecule *sub = new SubMolecule (NULL);
    sub->m_dummyParent->clear();
#if QT_VERSION >= 0x040700
    sub->m_atoms.reserve(this->numAtoms());
    sub->m_bonds.reserve(this->numBonds());
#endif // QT_VERSION

    // Copy bonds and atoms:
    // Lookup table: key = old id, val = new id.
    QHash<unsigned long, unsigned long> atomIdLUT;
    Q_ASSERT_X(m_atomList.size() == conf->size(), Q_FUNC_INFO,
               "atomList / conformer size mismatch.");
    for (int i = 0; i < m_atomList.size(); ++i) {
      Atom *oldAtom = m_atomList[i];
      Atom *newAtom = sub->m_dummyParent->addAtom();

      *newAtom = *oldAtom;
      newAtom->setPos((*conf)[i]);
      atomIdLUT.insert(oldAtom->id(), i);
      sub->m_atoms.append(newAtom);
    }

    for (int i = 0; i < m_bondList.size(); ++i) {
      Bond *oldBond = m_bondList[i];
      Bond *newBond = sub->m_dummyParent->addBond();

      int begInd = atomIdLUT.value(oldBond->beginAtomId());
      int endInd = atomIdLUT.value(oldBond->endAtomId());

      newBond->setBegin(sub->m_atoms[begInd]);
      newBond->setEnd(  sub->m_atoms[endInd]);
      newBond->setOrder(oldBond->order());

      sub->m_bonds.append(newBond);
    }

    sub->setSourceId(this->m_sourceId);

    return sub;
  }

  // Returns a new random representation of the submolecule
  SubMolecule *SubMoleculeSource::getRandomSubMolecule()
  {
    // Grab conformer index
    unsigned int confInd = this->weightedRandomConformerIndex();
    std::vector<Eigen::Vector3d> *conf = this->conformer(confInd);

    // Create submolecule
    SubMolecule *sub = new SubMolecule (NULL);
    sub->m_dummyParent->clear();
#if QT_VERSION >= 0x040700
    sub->m_atoms.reserve(this->numAtoms());
    sub->m_bonds.reserve(this->numBonds());
#endif // QT_VERSION

    // Copy bonds and atoms:
    // Lookup table: key = old id, val = new id.
    QHash<unsigned long, unsigned long> atomIdLUT;
    Q_ASSERT_X(m_atomList.size() == conf->size(), Q_FUNC_INFO,
               "atomList / conformer size mismatch.");
    for (int i = 0; i < m_atomList.size(); ++i) {
      Atom *oldAtom = m_atomList[i];
      Atom *newAtom = sub->m_dummyParent->addAtom();

      *newAtom = *oldAtom;
      newAtom->setPos((*conf)[i]);
      atomIdLUT.insert(oldAtom->id(), i);
      sub->m_atoms.append(newAtom);
    }

    for (int i = 0; i < m_bondList.size(); ++i) {
      Bond *oldBond = m_bondList[i];
      Bond *newBond = sub->m_dummyParent->addBond();

      int begInd = atomIdLUT.value(oldBond->beginAtomId());
      int endInd = atomIdLUT.value(oldBond->endAtomId());

      newBond->setBegin(sub->m_atoms[begInd]);
      newBond->setEnd(  sub->m_atoms[endInd]);
      newBond->setOrder(oldBond->order());

      sub->m_bonds.append(newBond);
    }

    // Randomize orientation
    const double angle = RANDDOUBLE() * M_PI;
    const Eigen::Vector3d axis = Eigen::Vector3d::Random();
    sub->rotate(angle, axis);

    // Center subMolecule
    sub->translate( - sub->center() );

    sub->setSourceId(this->m_sourceId);

    return sub;
  }

  void SubMoleculeSource::set(Avogadro::Molecule *mol)
  {
    this->clear();
    if (mol == NULL) {
      qDebug() << "Setting from NULL molecule in " << Q_FUNC_INFO;
      return;
    }

    this->setName(mol->fileName());

    const QList<Atom*> atoms = mol->atoms();
    const QList<Bond*> bonds = mol->bonds();

    // Lookup table: key = old id, val = new id.
    QHash<unsigned long, unsigned long> atomIdLUT;

    // Copy all atoms
    for (QList<Atom*>::const_iterator it = atoms.constBegin(),
         it_end = atoms.constEnd(); it != it_end; ++it) {
      Atom *atom = this->addAtom();
      *atom = **it;
      atomIdLUT.insert((*it)->id(), atom->id());
    }

    // Copy bonds
    for (QList<Bond*>::const_iterator it = bonds.constBegin(),
         it_end = bonds.constEnd(); it != it_end; ++it) {
      Bond *bond = this->addBond();
      Atom *begAtom = this->atomById(atomIdLUT.value((*it)->beginAtomId()));
      Atom *endAtom = this->atomById(atomIdLUT.value((*it)->endAtomId()));
      *bond = **it;
      bond->setBegin(begAtom);
      bond->setEnd(endAtom);
    }
  }

  void SubMoleculeSource::set(OpenBabel::OBMol *mol)
  {
    this->clear();
    if (mol == NULL) {
      qDebug() << "Setting from NULL molecule in " << Q_FUNC_INFO;
      return;
    }

    this->setName(QString::fromStdString(mol->GetFormula()));

    // Lookup table: key = old id, val = new id.
    QHash<unsigned long, unsigned long> atomIdLUT;

    // Copy all atoms
    FOR_ATOMS_OF_MOL (it, mol) {
      Atom *atom = this->addAtom();
      atom->setOBAtom(&*it);
      atomIdLUT.insert(it->GetId(), atom->id());
    }

    // Copy bonds
    FOR_BONDS_OF_MOL (it, mol) {
      Bond *bond = this->addBond();
      Atom *begAtom =
          this->atomById(atomIdLUT.value(it->GetBeginAtom()->GetId()));
      Atom *endAtom =
          this->atomById(atomIdLUT.value(it->GetEndAtom()->GetId()));
      bond->setOBBond(&*it);
      bond->setBegin(begAtom);
      bond->setEnd(endAtom);
    }
  }

  unsigned int SubMoleculeSource::findAndSetConformers()
  {
    // TODO Determine force field based on composition?

    // Remove any existing conformers
    this->clearConformers();

    // Create forcefield if needed:
    if (!setupForcefield() || m_ff == NULL) {
      qWarning() << "Cannot setup forcefield. Only the input conformer"
                    " will be used.";
      return 1;
    }

    // Lock the forcefield's mutex
    QMutexLocker locker (this->m_ffMutex);

    // Extract OBMol
    OpenBabel::OBMol obmol = this->OBMol();

    // Explicitly set the energy, otherwise the ff may crash due to OB bug
    std::vector<double> obenergies (this->energies());
    if (obenergies.size() == 0) {
      obenergies.push_back(0.0);
    }
    obmol.SetEnergies(obenergies);

    // Setup the forcefield with respect to the molecule
    if (!m_ff->Setup(obmol)) {
      qWarning() << "Cannot setup forcefield for this molecule. Only the "
                    "input conformer will be used.";
      return 1;
    }

    // Perform a preoptimization of the input geometry
    m_ff->ConjugateGradients(m_numGeoSteps, 1e-6);

    // Count the number of conformers that would be produced by a systematic
    // search
    int sysConfCount = m_ff->SystematicRotorSearchInitialize(m_numGeoSteps);

    // If the number of systematic conformers is less than the maximum, do
    // the systematic search
    if (sysConfCount <= this->m_maxConformers) {
      int confCount = 0;
      while (m_ff->SystematicRotorSearchNextConformer(m_numGeoSteps)) {
        ++confCount;
        emit conformerGenerated(confCount, sysConfCount);
      }
    }
    // Otherwise, just run a random conformer search
    else {
      m_ff->RandomRotorSearchInitialize(m_maxConformers);
      int confCount = 0;
      while (m_ff->RandomRotorSearchNextConformer(m_numGeoSteps)) {
        ++confCount;
        emit conformerGenerated(confCount, m_maxConformers);
      }
    }

    // Copy the conformers back from the forcefield into the OBMol
    //obmol = this->OBMol();
    m_ff->GetConformers(obmol);

    // Copy the conformers from OBMol into this SubMoleculeSource
    std::vector<Eigen::Vector3d> conformer;

    for (int i = 0; i < obmol.NumConformers(); ++i) {
      conformer.clear();
      conformer.reserve(obmol.NumAtoms());
      double *coordPtr = obmol.GetConformer(i);
      foreach (Atom *atom, m_atomList) {
        while (conformer.size() < atom->id())
          conformer.push_back(Eigen::Vector3d(0.0, 0.0, 0.0));
        conformer.push_back(Eigen::Vector3d(coordPtr));
        coordPtr += 3;
      }
      this->addConformer(conformer, i);
    }

    this->setEnergies(obmol.GetEnergies());

    this->sortConformersByEnergy();

    return this->numConformers();
  }

  bool SubMoleculeSource::setupForcefield()
  {
    QMutexLocker locker (this->m_ffMutex);

    // already setup!
    if (m_ff != NULL) {
      return true;
    }

    // Initialize forcefield. Try MMFF94s first, then UFF.
    if (!this->initializeMMFF94s()) {
      qWarning() << "Could not initialize MMFF94s force field. Attempting UFF.";
      if (!this->initializeUFF()) {
        qWarning() << "Could not initialize UFF force field. Aborting.";
        return false;
      }
      qWarning() << "Successfully setup UFF forcefield.";
    }
    return true;
  }

  bool SubMoleculeSource::initializeMMFF94s()
  {
    // An OBConverison object must be instantiated before the
    // FindForceField call will work.
    OpenBabel::OBConversion conv; Q_UNUSED(conv);
    OpenBabel::OBForceField *static_ff =
        OpenBabel::OBForceField::FindForceField("MMFF94s");
    if (!static_ff) {
      qWarning() << "Cannot locate MMFF94s forcefield.";
      return false;
    }
    OpenBabel::OBForceField *ff = static_ff->MakeNewInstance();
    if (!ff) {
      qWarning() << "Cannot clone MMFF94s forcefield.";
      return false;
    }

    ff->SetLogFile(NULL);
    ff->SetLogLevel(OBFF_LOGLVL_NONE);
    ff->SetLineSearchType(OpenBabel::LineSearchType::Newton2Num);

    OpenBabel::OBMol obmol = this->OBMol();
    if (!ff->Setup(obmol)) {
      qWarning() << "Cannot setup MMFF94s forcefield on this "
                    "SubMoleculeSource.";
      return false;
    }

    m_ff = ff;
    return true;
  }

  bool SubMoleculeSource::initializeUFF()
  {
    // An OBConverison object must be instantiated before the
    // FindForceField call will work.
    OpenBabel::OBConversion conv; Q_UNUSED(conv);
    OpenBabel::OBForceField *static_ff =
        OpenBabel::OBForceField::FindForceField("UFF");
    if (!static_ff) {
      qWarning() << "Cannot locate UFF forcefield.";
      return false;
    }
    OpenBabel::OBForceField *ff = static_ff->MakeNewInstance();
    if (!ff) {
      qWarning() << "Cannot clone UFF forcefield.";
      return false;
    }

    ff->SetLogFile(NULL);
    ff->SetLogLevel(OBFF_LOGLVL_NONE);
    ff->SetLineSearchType(OpenBabel::LineSearchType::Newton2Num);

    OpenBabel::OBMol obmol = this->OBMol();
    if (!ff->Setup(obmol)) {
      qWarning() << "Cannot setup UFF forcefield on this "
                    "SubMoleculeSource.";
      return false;
    }

    m_ff = ff;
    return true;
  }

  void SubMoleculeSource::sortConformersByEnergy()
  {
    // Grab list of conformers and energies
    std::vector<std::vector<Eigen::Vector3d>*> confs = this->conformers();
    std::vector<double> nrgs = this->energies();

    // Cache values, create temp vars for swapping
    unsigned int confCount = this->numConformers();
    double tmpd;
    std::vector<Eigen::Vector3d> *tmpv;

    // If any energy is infinite or nan, kick out the conformer. Calculate
    // the min, max, and average energies for further processing later.
    double min = DBL_MAX;
    double max = DBL_MIN;
    double ave = 0.0;
    for (size_t i = 0; i < static_cast<size_t>(confCount); ++i) {
      const double nrg = nrgs[i];
      if (GS_IS_NAN_OR_INF(nrg)) {
        // Decrease the number of known good conformers
        --confCount;
        // Move the last value to i. Molecule will clean up confs[i] when we
        // reset the conformers later.
        nrgs[i] = nrgs[confCount];
        confs[i] = confs[confCount];
        // Pop off the last entry
        confs.pop_back();
        nrgs.pop_back();
        // Recheck the new value at i
        --i;
      }
      else {
        if (nrg < min) min = nrg;
        if (nrg > max) max = nrg;
        ave += nrg;
      }
    }
    ave /= static_cast<double>(confCount);

    // The forcefield will occasionally produce these godawful conformers
    // that can only be the result of a bug. I'm talking about some REALLY
    // crazy geometries. Fortunately, the FFs do a good job of assigning these
    // an energy that is (appropriately) obnoxiously high. If a conformer's
    // energy is greater than ave + (ave - min), kick it out.
    const double maxEnergy = ave + (ave - min);
    for (size_t i = 0; i < static_cast<size_t>(confCount); ++i) {
      const double nrg = nrgs[i];
      if (nrg > maxEnergy) {
        // Decrease the number of known good conformers
        --confCount;
        // Move the last value to i. Molecule will clean up confs[i] when we
        // reset the conformers later.
        nrgs[i] = nrgs[confCount];
        confs[i] = confs[confCount];
        // Pop off the last entry
        confs.pop_back();
        nrgs.pop_back();
        // Recheck the new value at i
        --i;
      }
    }

    // Sort what's left over
    for (size_t i = 0; i < static_cast<size_t>(confCount-1); ++i) {
      double &e_i = nrgs[i];
      for (size_t j = i; j < static_cast<size_t>(confCount); ++j) {
        double &e_j = nrgs[j];
        if (e_j < e_i) {
          tmpd = nrgs[i];
          nrgs[i] = nrgs[j];
          nrgs[j] = tmpd;

          tmpv = confs[i];
          confs[i] = confs[j];
          confs[j] = tmpv;
        }
        e_i = nrgs[i];
      }
    }

    // Reassign conformers and energies
    Q_ASSERT(confs.size() == nrgs.size());
    this->setAllConformers(confs, false);
    this->setEnergies(nrgs);
  }

  unsigned int SubMoleculeSource::weightedRandomConformerIndex()
  {
    // Calculate probabilities for each conformer
    // TODO cache?

    // Handle special cases:
    if (this->numConformers() <= 1) {
      return 0;
    }
    // 75/25 split if only two conformers
    if (this->numConformers() == 2) {
      return (RANDDOUBLE() < 0.75) ? 0 : 1;
    }

    QVector<double> probs;
    probs.reserve(this->numConformers());

    // ASSUME that conformers are sorted by energy here:
    double lowest = this->energy(0);
    double highest = this->energy(this->numConformers()-1);
    double spread = highest - lowest;

    // If all conformers are at the same energy, lets save some time...
    if (spread <= 1e-5) {
      double dprob = 1.0/static_cast<double>(this->numConformers());
      double prob = 0;
      for (int i = 0; i < this->numConformers(); ++i) {
        probs.append(prob);
        prob += dprob;
      }
    }
    else {
      // Generate a list of floats from 0->1 proportional to the energies;
      // E.g. if energies are:
      // -5   -2   -1   3   5
      // We'll have:
      // 0   0.3  0.4  0.8  1
      for (int i = 0; i < this->numConformers(); ++i) {
        probs.append( ( this->energy(i) - lowest ) / spread);
      }
      // Subtract each value from one, and find the sum of the resulting list
      // We'll end up with:
      // 1  0.7  0.6  0.2  0   --   sum = 2.5
      double sum = 0;
      for (int i = 0; i < probs.size(); i++){
        probs[i] = 1.0 - probs.at(i);
        sum += probs.at(i);
      }
      // Give the last conformer the same probability as the one before it
      // (otherwise it will have 0 probability)
      // 1 0.7 0.6 0.2 0.2 -- sum = 2.7
      probs[probs.size()-1] = probs[probs.size()-2];
      sum += probs.last();
      // Normalize with the sum so that the list adds to 1
      // 0.37  0.26  0.22  0.07 0.07
      for (int i = 0; i < probs.size(); i++){
        probs[i] /= sum;
      }
      // Then replace each entry with a cumulative total:
      // 0.37 0.62 0.85 .92 1.0
      sum = 0;
      for (int i = 0; i < probs.size(); i++){
        sum += probs.at(i);
        probs[i] = sum;
      }
    }

    // Select an index
    double r = RANDDOUBLE();
    for (int i = 0; i < probs.size(); ++i) {
      if (r > probs[i]) {
        return static_cast<unsigned int>(i);
      }
    }

    // Failsafe -- return last conformer
    return this->numConformers()-1;
  }

} // end namespace XtalOpt
