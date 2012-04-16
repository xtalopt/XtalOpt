/**********************************************************************
  ExampleSearchGAMESS - Tools to interface with GAMESS remotely

  Copyright (C) 2012 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifndef EXAMPLESEARCHGAMESS_H
#define EXAMPLESEARCHGAMESS_H

#include <globalsearch/optimizer.h>

namespace ExampleSearch {

  class GAMESSOptimizer : public GlobalSearch::Optimizer
  {
    Q_OBJECT

   public:
    explicit GAMESSOptimizer(GlobalSearch::OptBase *parent,
                             const QString &filename = "");

  };

} // end namespace ExampleSearch

#endif
