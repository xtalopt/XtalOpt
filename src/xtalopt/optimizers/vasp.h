/**********************************************************************
  VASPOptimizer - Tools to interface with VASP

  Copyright (C) 2009-2011 by David C. Lonie

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

#include <QtCore/QObject>

namespace GlobalSearch {
  class Structure;
  class OptBase;
}

namespace XtalOpt {
  class VASPOptimizer : public XtalOptOptimizer
  {
    Q_OBJECT

   public:
    VASPOptimizer(GlobalSearch::OptBase *parent,
                  const QString &filename = "");

    QHash<QString, QString>
      getInterpretedTemplates(GlobalSearch::Structure *structure);

    void readSettings(const QString &filename = "");
    void writeTemplatesToSettings(const QString &filename = "");
    void writeDataToSettings(const QString &filename = "");

    void buildPOTCARs();
    bool POTCARInfoIsUpToDate(QList<uint> comp);
  };

} // end namespace XtalOpt

#endif
