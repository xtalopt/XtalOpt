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

#include "xtalopt.h"
#include "xtal.h"

#include <QObject>

namespace Avogadro {

  bool Optimizer::writeInputFiles(Xtal* xtal, XtalOpt *p) {
    switch (p->optType) {
    case XtalOpt::OptType_VASP:
      return XtalOptVASP::writeInputFiles(xtal, p);
      break;
    case XtalOpt::OptType_GULP:
      return XtalOptGULP::writeInputFiles(xtal, p);
      break;
    case XtalOpt::OptType_PWscf:
      return XtalOptPWscf::writeInputFiles(xtal, p);
      break;
    default: //Shouldn't happen...
      p->warning(tr("Optimizer::writeInputFiles: Invalid OptType: %1").arg(p->optType));
      return false;
      break;
    }
  }

  bool Optimizer::startOptimization(Xtal* xtal, XtalOpt *p) {
    switch (p->optType) {
    case XtalOpt::OptType_VASP:
      return XtalOptVASP::startOptimization(xtal, p);
      break;
    case XtalOpt::OptType_GULP:
      return XtalOptGULP::startOptimization(xtal, p);
      break;
    case XtalOpt::OptType_PWscf:
      return XtalOptPWscf::startOptimization(xtal, p);
      break;
    default: //Shouldn't happen...
      p->warning(tr("Optimizer::startOptimization: Invalid OptType: %1").arg(p->optType));
      return false;
      break;
    }
  }

  Optimizer::JobState Optimizer::getStatus(Xtal* xtal,
                                           XtalOpt *p) {
    switch (p->optType) {
    case XtalOpt::OptType_VASP:
      return XtalOptVASP::getStatus(xtal, p);
      break;
    case XtalOpt::OptType_GULP:
      return XtalOptGULP::getStatus(xtal, p);
      break;
    case XtalOpt::OptType_PWscf:
      return XtalOptPWscf::getStatus(xtal, p);
      break;
    default: //Shouldn't happen...
      p->warning(tr("Optimizer::getStatus: Invalid OptType: %1").arg(p->optType));
      return Error;
      break;
    }
  }

  bool Optimizer::deleteJob(Xtal *xtal, XtalOpt *p) {
    switch (p->optType) {
    case XtalOpt::OptType_VASP:
      return XtalOptVASP::deleteJob(xtal, p);
      break;
    case XtalOpt::OptType_GULP:
      return XtalOptGULP::deleteJob(xtal, p);
      break;
    case XtalOpt::OptType_PWscf:
      return XtalOptPWscf::deleteJob(xtal, p);
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

  int Optimizer::checkIfJobNameExists(Xtal* xtal, XtalOpt *p, const QStringList & queueData, bool & exists) {
    switch (p->optType) {
    case XtalOpt::OptType_VASP:
      return XtalOptVASP::checkIfJobNameExists(xtal, queueData, exists);
      break;
    case XtalOpt::OptType_GULP:
      return XtalOptGULP::checkIfJobNameExists(xtal, queueData, exists);
      break;
    case XtalOpt::OptType_PWscf:
      return XtalOptPWscf::checkIfJobNameExists(xtal, queueData, exists);
      break;
    default: //Shouldn't happen...
      p->warning(tr("Optimizer::checkIfJobNameExists: Invalid OptType: %1").arg(p->optType));
      return 0;
      break;
    }
  }

  bool Optimizer::updateXtal(Xtal* xtal,
                             XtalOpt *p) {
    switch (p->optType) {
    case XtalOpt::OptType_VASP:
      return XtalOptVASP::updateXtal(xtal, p);
      break;
    case XtalOpt::OptType_GULP:
      return XtalOptGULP::updateXtal(xtal, p);
      break;
    case XtalOpt::OptType_PWscf:
      return XtalOptPWscf::updateXtal(xtal, p);
      break;
    default: //Shouldn't happen...
      p->warning(tr("Optimizer::updateXtal: Invalid OptType: %1").arg(p->optType));
      return false;
      break;
    }
  }

  bool Optimizer::loadXtal(Xtal* xtal,
                           XtalOpt *p) {
    switch (p->optType) {
    case XtalOpt::OptType_VASP:
      return XtalOptVASP::loadXtal(xtal, p);
      break;
    case XtalOpt::OptType_GULP:
      return XtalOptGULP::loadXtal(xtal, p);
      break;
    case XtalOpt::OptType_PWscf:
      return XtalOptPWscf::loadXtal(xtal, p);
      break;
    default: //Shouldn't happen...
      p->warning(tr("Optimizer::loadXtal: Invalid OptType: %1").arg(p->optType));
      return false;
      break;
    }
  }

  bool Optimizer::readXtal(Xtal* xtal,
                           XtalOpt *p,
                           const QString & s) {
    switch (p->optType) {
    case XtalOpt::OptType_VASP:
      return XtalOptVASP::readXtal(xtal, p, s);
      break;
    case XtalOpt::OptType_GULP:
      return XtalOptGULP::readXtal(xtal, p, s);
      break;
    case XtalOpt::OptType_PWscf:
      return XtalOptPWscf::readXtal(xtal, p, s);
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
