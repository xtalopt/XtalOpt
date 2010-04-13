/**********************************************************************
  Optimizer - Chooses the optimization code based on
               XtalOpt::optType.

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

#ifndef XTALOPTJOB_H
#define XTALOPTJOB_H

#include <QObject>
#include <QStringList>

namespace Avogadro {
  class Structure;
  class XtalOpt;

  class Optimizer : public QObject
  {
    Q_OBJECT

   public:

    enum JobState { Unknown = -1, Success, Error, Queued, Running, CommunicationError, Started, Pending};

    static bool writeInputFiles(Structure *s, XtalOpt *p);

    static bool startOptimization(Structure *s, XtalOpt *p);

    static bool getQueueList(XtalOpt *p, QStringList & queueData);

    static bool deleteJob(Structure *s, XtalOpt *p);

    static JobState getStatus(Structure *s, XtalOpt *p);

    /*
     * Checks the queueData list for the jobname (extracted from xtal->fileName + "/job.pbs")
     * and sets exists to true if the job name is found. Return value is the job ID.
     */
    static int checkIfJobNameExists(Structure *s, XtalOpt *p, const QStringList & queueData, bool & exists);

    // Updates an existing xtal
    static bool update(Structure *s, XtalOpt *p);

    // Populates a new xtal
    static bool load(Structure *s, XtalOpt *p);

    // Handles reading of files for both of the above functions
    static bool read(Structure *s, XtalOpt *p, const QString & filename);

    static int totalOptSteps(XtalOpt *p);

   signals:

   public slots:

   private slots:

   private:
  };

} // end namespace Avogadro

#endif
