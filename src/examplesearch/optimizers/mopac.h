/**********************************************************************
  MopacOptimizer - Tools to interface with Mopac

  Copyright (C) 2012 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifndef EXAMPLESEARCH_MOPACOPTIMIZER_H
#define EXAMPLESEARCH_MOPACOPTIMIZER_H

#include <globalsearch/optimizer.h>

namespace ExampleSearch {

  class MopacOptimizer : public GlobalSearch::Optimizer
  {
    Q_OBJECT

   public:
    explicit MopacOptimizer(GlobalSearch::OptBase *parent,
                            const QString &filename = "");

  };

} // end namespace ExampleSearch

#endif
