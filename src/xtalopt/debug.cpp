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

#include <globalsearch/eleminfo.h>
#include <globalsearch/structures/molecule.h>

#include <QDebug>
#include <QFile>
#include <QString>
#include <QTextStream>

using GlobalSearch::Vector3;
using GlobalSearch::Matrix3;

namespace XtalOptDebug {
void dumpPseudoPwscfOut(const XtalOpt::Xtal* xtal, const QString& filename)
{
  QFile file(filename + ".out");
  if (!file.open(QFile::WriteOnly)) {
    qWarning() << "Cannot open file" << filename + ".out"
               << " for writing.";
  }

  QTextStream out(&file);

  // Ensure that the .out extension is handled properly
  out << "Program PWSCF" << endl;

  // Set units to angstrom
  out << "     lattice parameter (a_0)   =       1.88971616463 a.u." << endl;

  // Set cell matrix
  out << "CELL_PARAMETERS (alat)" << endl;
  const Matrix3 m(xtal->unitCell().cellMatrix());
  out << m(0, 0) << " " << m(0, 1) << " " << m(0, 2) << endl;
  out << m(1, 0) << " " << m(1, 1) << " " << m(1, 2) << endl;
  out << m(2, 0) << " " << m(2, 1) << " " << m(2, 2) << endl;
  out << endl;

  // Atomic positions
  out << "ATOMIC_POSITIONS (crystal)" << endl;
  const std::vector<GlobalSearch::Atom> atoms(xtal->atoms());
  Vector3 fcoord;
  for (std::vector<GlobalSearch::Atom>::const_iterator it = atoms.begin(),
                                                       it_end = atoms.end();
       it != it_end; ++it) {
    fcoord = xtal->cartToFrac((*it).pos());
    out << QString(ElemInfo::getAtomicSymbol(it->atomicNumber()).c_str()) << " "
        << fcoord.x() << " " << fcoord.y() << " " << fcoord.z() << endl;
  }
}
}
