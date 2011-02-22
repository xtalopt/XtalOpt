/**********************************************************************
  GULPOptimizer - Tools to interface with GULP

  Copyright (C) 2009-2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifndef GAPC_GULPOPTIMIZER_H
#define GAPC_GULPOPTIMIZER_H

#include <globalsearch/optimizer.h>

#include <QtCore/QObject>

namespace GlobalSearch {
  class OptBase;
}

namespace GAPC {
  class GULPOptimizer : public GlobalSearch::Optimizer
  {
    Q_OBJECT

   public:
    GULPOptimizer(GlobalSearch::OptBase *parent,
                  const QString &filename = "");
  };

} // end namespace GAPC

#endif
