/**********************************************************************
  Cluster - Implementation of an atomic cluster

  Copyright (C) 2010 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifndef CLUSTER_H
#define CLUSTER_H

#include <globalsearch/structure.h>

#include <QtCore/QVariant>
#include <QtCore/QHash>

using namespace GlobalSearch;

namespace GAPC {
  class Cluster : public Structure
  {
    Q_OBJECT

   public:
    Cluster(QObject *parent = 0);
    virtual ~Cluster();

    static bool compareNearestNeighborDistributions(const QList<double> &d,
                                                    const QList<double> &f1,
                                                    const QList<double> &f2,
                                                    double decay,
                                                    double smear,
                                                    double *error);

   signals:

   public slots:
    void constructRandomCluster(const QHash<unsigned int, unsigned int> &comp, float minIAD, float maxIAD);
    void centerAtoms();
    QHash<QString, QVariant> getFingerprint() const;

   private slots:

   private:

  };

} // end namespace Avogadro

#endif
