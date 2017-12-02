/**********************************************************************
  Cluster - Implementation of an atomic cluster

  Copyright (C) 2010-2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef CLUSTER_H
#define CLUSTER_H

#include <globalsearch/structure.h>

#include <QHash>
#include <QVariant>

namespace GAPC {
class Cluster : public GlobalSearch::Structure
{
  Q_OBJECT

public:
  Cluster(QObject* parent = 0);
  virtual ~Cluster();

signals:

public slots:
  void constructRandomCluster(const QHash<unsigned int, unsigned int>& comp,
                              float minIAD, float maxIAD);
  void centerAtoms();
  QHash<QString, QVariant> getFingerprint() const;
  bool checkForExplosion(double rcut) const;
  void expand(double factor);

private slots:

private:
};

} // end namespace GAPC

#endif
