/**********************************************************************
  GULPOptimizer - Tools to interface with GULP

  Copyright (C) 2009-2010 by David C. Lonie

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

#include <xtalopt/optimizers/xtaloptoptimizer.h>

#include <QtCore/QObject>

namespace GlobalSearch {
  class Structure;
  class OptBase;
  class Optimizer;
}

namespace XtalOpt {
  class GULPOptimizer : public XtalOptOptimizer
  {
    Q_OBJECT

   public:
    GULPOptimizer(GlobalSearch::OptBase *parent, const QString &filename = "");

    bool writeInputFiles(GlobalSearch::Structure *structure);
    bool startOptimization(GlobalSearch::Structure *structure);
    bool getQueueList(QStringList & queueData, QMutex *mutex);
    Optimizer::JobState getStatus(GlobalSearch::Structure *structure);
    bool copyRemoteToLocalCache(GlobalSearch::Structure *structure);
    int checkIfJobNameExists(GlobalSearch::Structure *,
                             const QStringList &, bool &b) {
      b=false;return 0;};
    bool deleteJob(GlobalSearch::Structure *s) {return true;};
  };

} // end namespace Avogadro

#endif
