/**********************************************************************
  Optimizer - Generic optimizer interface

  Copyright (C) 2010 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include <xtalopt/optimizers/xtaloptoptimizer.h>
#include <xtalopt/structures/xtal.h>
#include <xtalopt/xtalopt.h>

#include <globalsearch/optbase.h>

#include <QFile>

#include <openbabel/obconversion.h>
#include <openbabel/mol.h>

using namespace std;
using namespace OpenBabel;
using namespace Eigen;

namespace XtalOpt {

  XtalOptOptimizer::XtalOptOptimizer(OptBase *parent, const QString &filename) :
    Optimizer(parent)
  {
  }

  XtalOptOptimizer::~XtalOptOptimizer()
  {
  }

  bool XtalOptOptimizer::read(Structure *structure,
                              const QString & filename) {
    // Recast structure as xtal -- we'll need to access cell data later
    Xtal *xtal = qobject_cast<Xtal*>(structure);
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);
    // Test filename
    QFile file (filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      return false;
    }
    file.close();

    // Read in OBMol
    //
    // OpenBabel::OBConversion;:ReadFile calls a singleton error class
    // that is not thread safe. Hence sOBMutex is necessary.
    m_opt->sOBMutex->lock();
    OBConversion conv;
    OBFormat* inFormat = conv.FormatFromExt(QString(QFile::encodeName(filename.trimmed())).toAscii());

    if ( !inFormat || !conv.SetInFormat( inFormat ) ) {
      m_opt->warning(tr("Optimizer::read: Error setting format for file %1")
                 .arg(filename));
      xtal->setStatus(Xtal::Error);
      m_opt->sOBMutex->unlock();
      return false;
    }

    OBMol obmol;
    conv.ReadFile( &obmol, QString(QFile::encodeName(filename)).toStdString());
    m_opt->sOBMutex->unlock();

    // Copy settings from obmol -> xtal.
    // cell
    OBUnitCell *cell = static_cast<OBUnitCell*>(obmol.GetData(OBGenericDataType::UnitCell));

    if (cell == NULL) {
      m_opt->warning(tr("Optimizer::read: No unit cell in %1? Weird...").arg(filename));
      xtal->setStatus(Xtal::Error);
      return false;
    }

    xtal->setCellInfo(cell->GetCellMatrix());

    // atoms
    while (xtal->numAtoms() < obmol.NumAtoms())
      xtal->addAtom();
    QList<Atom*> atoms = xtal->atoms();
    uint i = 0;

    FOR_ATOMS_OF_MOL(atm, obmol) {
      atoms.at(i)->setPos(Vector3d(atm->x(), atm->y(), atm->z()));
      atoms.at(i)->setAtomicNumber(atm->GetAtomicNum());
      i++;
    }

    // energy/enthalpy
    const double KCAL_PER_MOL_TO_EV = 0.0433651224;
    if (obmol.HasData("Enthalpy (kcal/mol)"))
      xtal->setEnthalpy(QString(obmol.GetData("Enthalpy (kcal/mol)")->GetValue().c_str()).toFloat()
                        * KCAL_PER_MOL_TO_EV);
    if (obmol.HasData("Enthalpy PV term (kcal/mol)"))
      xtal->setPV(QString(obmol.GetData("Enthalpy PV term (kcal/mol)")->GetValue().c_str()).toFloat()
                  * KCAL_PER_MOL_TO_EV);
    xtal->setEnergy(obmol.GetEnergy());
    // Modify as needed!

    xtal->wrapAtomsToCell();
    xtal->findSpaceGroup(xtalopt->tol_spg);
    return true;
  }

} // end namespace avogadro

//#include "xtaloptoptimizer.moc"
