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

#include "optimizers.h"

#include "optimizers/vasp.h"
#include "optimizers/gulp.h"
#include "optimizers/pwscf.h"

#include "structure.h"
#include "xtalopt.h"

#include <QObject>

namespace Avogadro {

  bool Optimizer::writeInputFiles(Structure *s, XtalOpt *p) {
    switch (p->optType) {
    case XtalOpt::OptType_VASP:
      return XtalOptVASP::writeInputFiles(s, p);
      break;
    case XtalOpt::OptType_GULP:
      return XtalOptGULP::writeInputFiles(s, p);
      break;
    case XtalOpt::OptType_PWscf:
      return XtalOptPWscf::writeInputFiles(s, p);
      break;
    default: //Shouldn't happen...
      p->warning(tr("Optimizer::writeInputFiles: Invalid OptType: %1").arg(p->optType));
      return false;
      break;
    }
  }

  bool Optimizer::startOptimization(Structure *s, XtalOpt *p) {
    switch (p->optType) {
    case XtalOpt::OptType_VASP:
      return XtalOptVASP::startOptimization(s, p);
      break;
    case XtalOpt::OptType_GULP:
      return XtalOptGULP::startOptimization(s, p);
      break;
    case XtalOpt::OptType_PWscf:
      return XtalOptPWscf::startOptimization(s, p);
      break;
    default: //Shouldn't happen...
      p->warning(tr("Optimizer::startOptimization: Invalid OptType: %1").arg(p->optType));
      return false;
      break;
    }
  }

  Optimizer::JobState Optimizer::getStatus(Structure *s,
                                           XtalOpt *p) {
    switch (p->optType) {
    case XtalOpt::OptType_VASP:
      return XtalOptVASP::getStatus(s, p);
      break;
    case XtalOpt::OptType_GULP:
      return XtalOptGULP::getStatus(s, p);
      break;
    case XtalOpt::OptType_PWscf:
      return XtalOptPWscf::getStatus(s, p);
      break;
    default: //Shouldn't happen...
      p->warning(tr("Optimizer::getStatus: Invalid OptType: %1").arg(p->optType));
      return Error;
      break;
    }
  }

  bool Optimizer::deleteJob(Structure *s, XtalOpt *p) {
    switch (p->optType) {
    case XtalOpt::OptType_VASP:
      return XtalOptVASP::deleteJob(s, p);
      break;
    case XtalOpt::OptType_GULP:
      return XtalOptGULP::deleteJob(s, p);
      break;
    case XtalOpt::OptType_PWscf:
      return XtalOptPWscf::deleteJob(s, p);
      break;
    default: //Shouldn't happen...
      p->warning(tr("Optimizer::deleteJob: Invalid OptType: %1").arg(p->optType));
      return false;
      break;
    }
  }

  bool Optimizer::getQueueList(XtalOpt *p,
                               QStringList & queueData) {
    switch (p->optType) {
    case XtalOpt::OptType_VASP:
      return XtalOptVASP::getQueueList(p, queueData);
      break;
    case XtalOpt::OptType_GULP:
      return XtalOptGULP::getQueueList(p, queueData);
      break;
    case XtalOpt::OptType_PWscf:
      return XtalOptPWscf::getQueueList(p, queueData);
      break;
    default: //Shouldn't happen...
      p->warning(tr("Optimizer::getQueueList: Invalid OptType: %1").arg(p->optType));
      return false;
      break;
    }
  }

  int Optimizer::checkIfJobNameExists(Structure *s, XtalOpt *p, const QStringList & queueData, bool & exists) {
    switch (p->optType) {
    case XtalOpt::OptType_VASP:
      return XtalOptVASP::checkIfJobNameExists(s, queueData, exists);
      break;
    case XtalOpt::OptType_GULP:
      return XtalOptGULP::checkIfJobNameExists(s, queueData, exists);
      break;
    case XtalOpt::OptType_PWscf:
      return XtalOptPWscf::checkIfJobNameExists(s, queueData, exists);
      break;
    default: //Shouldn't happen...
      p->warning(tr("Optimizer::checkIfJobNameExists: Invalid OptType: %1").arg(p->optType));
      return 0;
      break;
    }
  }

  bool Optimizer::update(Structure *s,
                             XtalOpt *p) {
    switch (p->optType) {
    case XtalOpt::OptType_VASP:
      return XtalOptVASP::update(s, p);
      break;
    case XtalOpt::OptType_GULP:
      return XtalOptGULP::update(s, p);
      break;
    case XtalOpt::OptType_PWscf:
      return XtalOptPWscf::update(s, p);
      break;
    default: //Shouldn't happen...
      p->warning(tr("Optimizer::updateXtal: Invalid OptType: %1").arg(p->optType));
      return false;
      break;
    }
  }

  bool Optimizer::load(Structure *s,
                           XtalOpt *p) {
    switch (p->optType) {
    case XtalOpt::OptType_VASP:
      return XtalOptVASP::load(s, p);
      break;
    case XtalOpt::OptType_GULP:
      return XtalOptGULP::load(s, p);
      break;
    case XtalOpt::OptType_PWscf:
      return XtalOptPWscf::load(s, p);
      break;
    default: //Shouldn't happen...
      p->warning(tr("Optimizer::loadXtal: Invalid OptType: %1").arg(p->optType));
      return false;
      break;
    }
  }

  bool Optimizer::read(Structure *s,
                           XtalOpt *p,
                           const QString & str) {
    switch (p->optType) {
    case XtalOpt::OptType_VASP:
      return XtalOptVASP::read(s, p, str);
      break;
    case XtalOpt::OptType_GULP:
      return XtalOptGULP::read(s, p, str);
      break;
    case XtalOpt::OptType_PWscf:
      return XtalOptPWscf::read(s, p, str);
      break;
    default: //Shouldn't happen...
      p->warning(tr("Optimizer::readXtal: Invalid OptType: %1").arg(p->optType));
      return false;
      break;
    }    
  }

  int Optimizer::totalOptSteps(XtalOpt *p) {
    switch (p->optType) {
    case XtalOpt::OptType_VASP:
      return XtalOptVASP::totalOptSteps(p);
      break;
    case XtalOpt::OptType_GULP:
      return XtalOptGULP::totalOptSteps(p);
      break;
    case XtalOpt::OptType_PWscf:
      return XtalOptPWscf::totalOptSteps(p);
      break;
    default: //Shouldn't happen...
      p->warning(tr("Optimizer::totalOptSteps: Invalid OptType: %1").arg(p->optType));
      return 0;
      break;
    }
  }

} // end namespace Avogadro

#include "optimizers.moc"
