/**********************************************************************
  XtalOptDebug - Some helpful tools for debugging XtalOpt

  Copyright (C) 2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <xtalopt/debug.h>

#include <xtalopt/structures/xtal.h>

#include <globalsearch/obeigenconv.h>

#include <avogadro/moleculefile.h>

#include <openbabel/data.h> // for etab

#include <QtCore/QFile>
#include <QtCore/QTextStream>

using namespace Avogadro;
using namespace OpenBabel;

namespace XtalOptDebug
{
  void dumpCML(const XtalOpt::Xtal *xtal, const QString &filename)
  {
    Avogadro::MoleculeFile::writeMolecule(xtal, filename + ".cml");
  }

  void dumpPseudoPwscfOut(const XtalOpt::Xtal *xtal, const QString &filename)
  {
    QFile file (filename + ".out");
    if (!file.open(QFile::WriteOnly)) {
      qWarning() << "Cannot open file" << filename + ".out" << " for writing.";
    }

    QTextStream out (&file);

    // Ensure that the .out extension is handled properly
    out << "Program PWSCF" << endl;

    // Set units to angstrom
    out << "     lattice parameter (a_0)   =       1.88971616463 a.u." << endl;

    // Set cell matrix
    out << "CELL_PARAMETERS (alat)" << endl;
    const Eigen::Matrix3d m (OB2Eigen(xtal->OBUnitCell()->GetCellMatrix()));
    out << m(0,0) << " " << m(0,1) << " " << m(0,2) << endl;
    out << m(1,0) << " " << m(1,1) << " " << m(1,2) << endl;
    out << m(2,0) << " " << m(2,1) << " " << m(2,2) << endl;
    out << endl;

    // Atomic positions
    out << "ATOMIC_POSITIONS (crystal)" << endl;
    const QList<Atom*> atoms (xtal->atoms());
    Eigen::Vector3d fcoord;
    for (QList<Atom*>::const_iterator it = atoms.constBegin(),
           it_end = atoms.constEnd(); it != it_end; ++it) {
      fcoord = xtal->cartToFrac(*(*it)->pos());
      out << etab.GetSymbol((*it)->atomicNumber()) << " "
          << fcoord.x() << " "
          << fcoord.y() << " "
          << fcoord.z() << endl;
    }
  }
}
