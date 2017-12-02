/**********************************************************************
  RandomDock - Matrix: Wrapper for Avogadro::Molecule to hold the
  matrix monomers in a docking problem

  Copyright (C) 2009-2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef MATRIXMOL_H
#define MATRIXMOL_H

#include <globalsearch/structure.h>

#include <QDateTime>
#include <QTextStream>

namespace RandomDock {

class Matrix : public GlobalSearch::Structure
{
  Q_OBJECT

public:
  Matrix(QObject* parent = 0);
  Matrix(Avogadro::Molecule* mol);
  virtual ~Matrix();

signals:

public slots:
  double prob(uint index)
  {
    checkProbs();
    return m_probs.at(index);
  };
  void sortConformers();
  void generateProbabilities();
  void checkProbs()
  {
    if ((uint)m_probs.size() != numConformers())
      generateProbabilities();
  };
  int getRandomConformerIndex();

private slots:

private:
  QList<double> m_probs;
};

} // end namespace RandomDock

#endif
