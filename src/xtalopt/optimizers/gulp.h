/**********************************************************************
  GULPOptimizer - Tools to interface with GULP

  Copyright (C) 2009-2010 by David C. Lonie

  This file is part of the Avogadro molecular editor project.
  For more information, see <http://avogadro.openmolecules.net/>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifndef GULPOPTIMIZER_H
#define GULPOPTIMIZER_H

#include "../../generic/optimizer.h"

#include <QObject>

namespace Avogadro {
  class Structure;
  class OptBase;

  class GULPOptimizer : public Optimizer
  {
    Q_OBJECT

   public:
    GULPOptimizer(OptBase *parent);

    bool writeInputFiles(Structure *structure);
    bool startOptimization(Structure *structure);
    bool getQueueList(QStringList & queueData);
    Optimizer::JobState getStatus(Structure *structure);
    bool copyRemoteToLocalCache(Structure *structure);
    int checkIfJobNameExists(Structure *, const QStringList &, bool &b) {
      b=false;return 0;};
  };

} // end namespace Avogadro

#endif
