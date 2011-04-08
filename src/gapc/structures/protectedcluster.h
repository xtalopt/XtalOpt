/**********************************************************************
  ProtectedCluster - Contains a cluster with protecting ligands

  Copyright (C) 2010-2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifndef PROTECTEDCLUSTER_H
#define PROTECTEDCLUSTER_H

#include <gapc/structures/cluster.h>

namespace GAPC {
  class ProtectedCluster : public Cluster
  {
    Q_OBJECT

   public:
    ProtectedCluster(QObject *parent = 0);
    virtual ~ProtectedCluster();

   signals:

   public slots:

   private slots:

   private:

  };

}

#endif
