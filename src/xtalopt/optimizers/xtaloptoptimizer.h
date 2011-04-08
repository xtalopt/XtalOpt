/**********************************************************************
  XtalOptOptimizer - Generic optimizer interface

  Copyright (C) 2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifndef XTALOPTOPTIMIZER_H
#define XTALOPTOPTIMIZER_H

#include <globalsearch/optimizer.h>

#include <QtCore/QObject>

namespace GlobalSearch {
  class Structure;
  class OptBase;
  class Optimizer;
}

namespace XtalOpt {

  class XtalOptOptimizer : public GlobalSearch::Optimizer
  {
    Q_OBJECT

  public:
    explicit XtalOptOptimizer(GlobalSearch::OptBase *parent,
                              const QString &filename = "");
    virtual ~XtalOptOptimizer();

    virtual bool read(GlobalSearch::Structure *structure, const QString & filename);

  };

} // end namespace XtalOpt

#endif
