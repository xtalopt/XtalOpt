/**********************************************************************
  Xtal - XtalOpt-specific Structure subclass.

  Copyright (C) 2009-2011 by David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef XTAL_H
#define XTAL_H

#include <search/structure.h>
#include <common/constants.h>

#include <xtalopt/types.h>

#include <QMutex>
#include <QList>

class QFile;

namespace XtalOpt {

// XtalOpt structure information and checks.
class Xtal : public Search::Structure
{
  Q_OBJECT

public:
  Xtal(QObject* parent = nullptr);
  Xtal(double A, double B, double C, double Alpha, double Beta, double Gamma,
       QObject* parent = nullptr);

  /* Copy constructor */
  Xtal(const Xtal& other);

  /* Move constructor */
  Xtal(Xtal&& other) noexcept;

  /* Assignment operator */
  Xtal& operator=(const Xtal& other);

  /* Move assignment operator */
  Xtal& operator=(Xtal&& other) noexcept;

  virtual ~Xtal() override;

  /** Functions to set or retrieve the "composition validity"
   *  for a structure. Validity here means that the structure
   *  has a chemical composition that matches one of the user's
   *  input formulas list, or -at least- is a supercell of one of
   *  those formulas. So, e.g., a "sub-system" seed structure
   *  will not have a valid composition.
   *  For now, this is being used in genetic operation selection
   *  to filter out structures with "unknown" comp from stripple/permustrain
   *  in a fixed/multi composition search, where we already
   *  know the output of gen opt will not be acceptable.
   *  By default, this is set to true as internally generated
   *  structures match the list (or in vc search are acceptable
   *  anyways). Currently, we only set it to "false" in the
   *  "checkComposition" for off-composition seeds.
   */
  bool hasValidComposition() const { return m_hasValidComposition; };
  void setCompositionValidity(bool value) { m_hasValidComposition = value; };

  bool addAtomRandomly(uint atomicNumber, double minIAD = 0.0,
                       int maxAttempts = 100) override;
  // Uses the minRadius constraints in @a limits to restrict atom placement
  bool addAtomRandomly(unsigned int atomicNumber,
                       const EleRadii& limits,
                       int maxAttempts = 100.0);
  bool moveAtomRandomly(unsigned int atomicNumber, const EleRadii& limits,
    int maxAttempts = 100.0, Atoms::Atom* atom = nullptr);

  bool addAtomRandomlyIAD(unsigned int atomicNumber,
    const QHash<QPair<int, int>, IAD>& limitsIAD, int maxAttempts = 100.0);
  bool moveAtomRandomlyIAD(unsigned int atomicNumber,
    const QHash<QPair<int, int>, IAD>& limitsIAD, int maxAttempts = 100.0,
    Atoms::Atom* atom = nullptr);
  bool checkMinIAD(const QHash<QPair<int, int>, IAD>& limitsIAD,
                   int* atom1 = nullptr, int* atom2 = nullptr,
                   double* IAD = nullptr);

  // Use the minRadius constraints in @a limits to check the interatomic
  // distances in the xtal. atom1 and atom2 are overwritten with the indexes
  // of the first set of offending atom, if any, that are found. The bad IAD
  // is written to IAD if a double pointer is provided.
  bool checkInteratomicDistances(
    const EleRadii& limits,
    int* atom1 = nullptr, int* atom2 = nullptr, double* IAD = nullptr);
  virtual QString getResultsEntry(int objectives_num, int optstep, int objective_offset = 0,
                                  int constraints_num = 0) const override;
  virtual QString getResultsHeader(int objectives_num, int objective_offset = 0,
                                   int constraints_num = 0) const override
  {
    Q_UNUSED(objective_offset);
    QString out = QString("%1 %2 %3 %4 %5 %6 %7 %8")
        .arg("Rank", 5)
        .arg("Struct", 9)
        .arg("Formula", 15)
        .arg("Compos", 10)
        .arg("Index", 5)
        .arg("EnthalpyAtm", 12)
        .arg("Front", 5)
        .arg("AbovHullAtm", 12);
    for (int i = 0; i < objectives_num; i++)
      out += QString("%1").arg("Objc"+QString::number(i+1), 11);
    for (int i = 0; i < constraints_num; i++)
      out += QString("%1").arg(QString("Cons%1").arg(i + 1), 6);
    out += QString("%1 %2")
        .arg("SG", 6)
        .arg("Status", 16);

    return out;
  };

  /** Functions to set or retrieve the distance above hull
   *    (in energy per atom units).
   */
  double getDistAboveHull() const { return m_aboveHull; };
  void setDistAboveHull(double value) { m_aboveHull = value; };

  static QString getHullHeader(const QList<QString>& chemSystem);
  QString getHullEntry(const QList<QString>& chemSystem) const;

  // Randomly skew the lattice and translate the coordinates. Coordinates
  // may be reflected, but the structures should be energetically
  // equivalent
  Xtal* getRandomRepresentation() const;


  bool operator==(const Xtal& other) const;
  bool operator!=(const Xtal& other) const { return !operator==(other); };

  // For random representation generation
  static void generateValidCOBs();
  static const QList<Common::Matrix3>& transformationMatrices()
  {
    return m_transformationMatrices;
  }
  static const QList<Common::Matrix3>& mixMatrices()
  {
    return m_mixMatrices;
  }

public slots:
  // Self-correction
  bool fixAngles(int attempts = 100);

  // Printing debug output
  void printLatticeInfo() const;
  void printAtomInfo() const;
  void printXtalInfo() const;

private slots:

private:
  static QList<Common::Matrix3> m_transformationMatrices;
  static QList<Common::Matrix3> m_mixMatrices;
  // Ensure that only one thread generates the COB vectors
  static QMutex m_validCOBsGenMutex;
  bool m_hasValidComposition;
  double m_aboveHull;
};

} // end namespace XtalOpt

#endif
