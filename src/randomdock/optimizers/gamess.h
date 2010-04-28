/**********************************************************************
  RandomDockGAMESS - Tools to interface with GAMESS remotely

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

#ifndef RANDOMDOCKGAMESS_H
#define RANDOMDOCKGAMESS_H

#include <openbabel/generic.h>
#include <openbabel/obiter.h>

#include <avogadro/molecule.h>

namespace Avogadro {
  class RandomDockParams;
  class Scene;

  class RandomDockGAMESS : public QObject
  {
    Q_OBJECT

   public:

    enum GAMESSState	{ Unknown = -1, Success, Error, Queued, Running, CommunicationError};

    static bool writeInputFiles(Scene *scene, RandomDockParams *p);
    static bool readOutputFiles(Scene *scene, RandomDockParams *p);
    static bool stripOutputFile(Scene *scene, RandomDockParams *p);
    static bool copyToRemote(Scene *scene, RandomDockParams *p);
    static bool copyFromRemote(Scene *scene, RandomDockParams *p);

    static bool getQueue(RandomDockParams *p, QStringList & queuedata);
    static int getStatus(Scene *scene, RandomDockParams *p, const QStringList & queuedata);

    static bool startJob(Scene *scene, RandomDockParams *p);
    static bool deleteJob(Scene *scene, RandomDockParams *p);

   signals:

   public slots:

   private slots:

   private:
  };

} // end namespace Avogadro

#endif
