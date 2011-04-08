/**********************************************************************
  Cluster - Implementation of an atomic cluster

  Copyright (C) 2010-2011 by David C. Lonie

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

namespace GAPC {
  class Cluster : public GlobalSearch::Structure
  {
    Q_OBJECT

   public:
    Cluster(QObject *parent = 0);
    virtual ~Cluster();

   signals:

   public slots:
    void constructRandomCluster(const QHash<unsigned int, unsigned int> &comp, float minIAD, float maxIAD);
    void centerAtoms();
    QHash<QString, QVariant> getFingerprint() const;
    bool checkForExplosion(double rcut) const;
    void expand(double factor);

   private slots:

   private:

  };

} // end namespace GAPC

#endif
