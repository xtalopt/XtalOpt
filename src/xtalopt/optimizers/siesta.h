/**********************************************************************
  SIESTAOptimizer - Tools to interface with SIESTA

  Copyright (C) 2009-2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifndef SIESTAOPTIMIZER_H
#define SIETSAOPTIMIZER_H

#include <xtalopt/optimizers/xtaloptoptimizer.h>

#include <QtCore/QObject>

namespace GlobalSearch {
  class Structure;
  class OptBase;
  class Optimizer;
}

namespace XtalOpt {
  class SIESTAOptimizer : public XtalOptOptimizer
  {
    Q_OBJECT

   public:
    SIESTAOptimizer(GlobalSearch::OptBase *parent,
                  const QString &filename = "");

    QHash<QString, QString>
      getInterpretedTemplates(GlobalSearch::Structure *structure);


    void buildPSFs();
    bool PSFInfoIsUpToDate(QList<uint> comp);
  };

} // end namespace XtalOpt

#endif
