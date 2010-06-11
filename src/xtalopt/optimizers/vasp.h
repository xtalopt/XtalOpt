/**********************************************************************
  VASPOptimizer - Tools to interface with VASP

  Copyright (C) 2009-2010 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifndef VASPOPTIMIZER_H
#define VASPOPTIMIZER_H

#include <xtalopt/optimizers/xtaloptoptimizer.h>

#include <QObject>

namespace GlobalSearch {
  class Structure;
  class OptBase;
}

using namespace GlobalSearch;

namespace XtalOpt {
  class VASPOptimizer : public XtalOptOptimizer
  {
    Q_OBJECT

   public:
    VASPOptimizer(OptBase *parent, const QString &filename = "");
    bool writeInputFiles(Structure *structure);
    void readSettings(const QString &filename = "");
    void writeTemplatesToSettings(const QString &filename = "");
    void writeDataToSettings(const QString &filename = "");

    void buildPOTCARs();
    bool POTCARInfoIsUpToDate(QList<uint> comp);
  };

} // end namespace Avogadro

#endif
