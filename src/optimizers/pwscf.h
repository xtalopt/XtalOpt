/**********************************************************************
  XtalOptPWscf - Tools to interface with PWscf remotely

  Copyright (C) 2009 by David C. Lonie

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

#ifndef XTALOPTPWSCF_H
#define XTALOPTPWSCF_H

#include "../optimizers.h"

#include <QObject>

namespace Avogadro {
  class Structure;
  class XtalOpt;

  class XtalOptPWscf : public QObject
  {
    Q_OBJECT

   public:

    static bool writeInputFiles(Structure *structure, XtalOpt *p);

    static bool startOptimization(Structure *structure, XtalOpt *p);

    static bool getOutputFile(const QString & path,
                              const QString & filename,
                              QStringList & data,
                              const QString & host = "",
                              const QString & username = "");

    static bool copyRemoteToLocalCache(Structure *structure, XtalOpt *p);

    static bool getQueueList(XtalOpt *p, QStringList & queueData);

    static bool deleteJob(Structure *structure, XtalOpt *p);

    static Optimizer::JobState getStatus(Structure *structure, XtalOpt *p);

    /*
     * Checks the queueData list for the jobname (extracted from xtal->fileName + "/job.pbs")
     * and sets exists to true if the job name is found. Return value is the job ID.
     */
    static int checkIfJobNameExists(Structure *structure, const QStringList & queueData, bool & exists);

    // Updates an existing xtal
    static bool update(Structure *structure, XtalOpt *p);

    // Populates a new xtal
    static bool load(Structure *structure, XtalOpt *p);

    // Handles reading of files for both of the above functions
    static bool read(Structure *structure, XtalOpt *p, const QString & filename);

    static int totalOptSteps(XtalOpt *p);

   signals:

   public slots:

   private slots:

   private:
  };

} // end namespace Avogadro

#endif
