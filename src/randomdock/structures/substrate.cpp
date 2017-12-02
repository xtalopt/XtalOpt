/**********************************************************************
  RandomDock - Substrate: Wrapper for Avogadro::Molecule to hold the
  central molecule in a docking problem

  Copyright (C) 2009-2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <randomdock/structures/substrate.h>

#include <globalsearch/macros.h>
#include <globalsearch/structure.h>

#include <openbabel/mol.h>

#include <QDebug>

#include <QProgressDialog>

using namespace std;
using namespace Avogadro;

namespace RandomDock {

Substrate::Substrate(QObject* parent) : Structure(parent)
{
}

Substrate::Substrate(Molecule* mol) : Structure(*mol)
{
  generateProbabilities();
}

Substrate::~Substrate()
{
}

void Substrate::sortConformers()
{
  std::vector<Eigen::Vector3d> tmp;
  double tmp_e;

  // Make sure that energies().size() == numConformers:
  energies();

  QProgressDialog prog("Sorting conformers by energy...", 0, 0, numConformers(),
                       NULL);

  // Simple selection sort
  for (uint i = 0; i < numConformers(); i++) {
    prog.setValue(i);
    for (uint j = i; j < numConformers(); j++) {
      if (energy(j) < energy(i)) {
        tmp = *conformer(i);
        addConformer(*conformer(j), i);
        addConformer(tmp, j);
        tmp_e = energy(i);
        setEnergy(i, energy(j));
        setEnergy(j, tmp_e);
      }
    }
  }
}

void Substrate::generateProbabilities()
{
  if (numConformers() == 1) {
    m_probs.clear();
    m_probs.append(1.0);
    return;
  }

  sortConformers();
  m_probs.clear();
  double lowest = energy(0);
  ;
  double highest = energy(numConformers() - 1);
  double spread = highest - lowest +
                  (highest - lowest) / static_cast<double>(numConformers());
  // Generate a list of floats from 0->1 proportional to the energies;
  // E.g. if energies are:
  // -5   -2   -1   3   5
  // We'll have:
  // 0   0.3  0.4  0.8  1
  for (uint i = 0; i < numConformers(); i++)
    m_probs.append((energy(i) - lowest) / spread);
  // Subtract each value from one, and find the sum of the resulting list
  // We'll end up with:
  // 1  0.7  0.6  0.2  0  --   sum = 2.5
  double sum = 0;
  for (int i = 0; i < m_probs.size(); i++) {
    m_probs.replace(i, 1.0 - m_probs.at(i));
    sum += m_probs.at(i);
  }
  // Normalize with the sum so that the list adds to 1
  // 0.4  0.28  0.24  0.08  0
  for (int i = 0; i < m_probs.size(); i++) {
    m_probs.replace(i, m_probs.at(i) / sum);
  }
  // Then replace each entry with a cumulative total:
  // 0.4 0.68 0.92 1 1
  sum = 0;
  for (int i = 0; i < m_probs.size(); i++) {
    sum += m_probs.at(i);
    m_probs.replace(i, sum);
  }
  // And we have a energy weighted probability list! To use:
  //
  //   double r = RANDDOUBLE();
  //   uint ind;
  //   for (ind = 0; ind < m_probs.size(); ind++)
  //     if (r < m_probs.at(ind)) break;
  //
  // ind will hold the chosen index.
  //
  // Alternatively, the percent probability can be recovered as such:
  //
  //   QList<double> percents;
  //   for (int i = 0; i < m_probs.size(); i++) {
  //     if (i == 0) percents.append(m_probs.at(i) * 100.0);
  //     else percents.append( ( m_probs.at(i) - m_probs.at(i-1) ) * 100.0 )
  //   }
  //
  // percents will hold the percent probabilities
}

int Substrate::getRandomConformerIndex()
{
  INIT_RANDOM_GENERATOR();
  // Select conformer to use:
  double r = RANDDOUBLE();
  int ind;
  for (ind = 0; ind < m_probs.size(); ind++)
    if (r < m_probs.at(ind))
      break;

  return r;
}

} // end namespace RandomDock
