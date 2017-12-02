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

#ifndef SUBSTRATEMOL_H
#define SUBSTRATEMOL_H

#include <globalsearch/structure.h>

#include <QDateTime>
#include <QDebug>
#include <QTextStream>

namespace RandomDock {

class Substrate : public GlobalSearch::Structure
{
  Q_OBJECT

public:
  Substrate(QObject* parent = 0);
  Substrate(Avogadro::Molecule* mol);
  virtual ~Substrate();

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
