/**********************************************************************
  GuassianOptimizer - Tools to interface with Gaussian

  Copyright (C) 2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifndef RD_GAUSSIANOPTIMIZER_H
#define RD_GAUSSIANOPTIMIZER_H

#include <globalsearch/optimizer.h>

namespace RandomDock {

  class GaussianOptimizer : public GlobalSearch::Optimizer
  {
    Q_OBJECT

   public:
    explicit GaussianOptimizer(GlobalSearch::OptBase *parent,
                            const QString &filename = "");

  };

} // end namespace RandomDock

#endif
