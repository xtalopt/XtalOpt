/**********************************************************************
  ADFOptimizer - Tools to interface with ADF remotely

  Copyright (C) 2010-2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifndef RDADF_H
#define RDADF_H

#include <globalsearch/optimizer.h>

namespace GlobalSearch {
  class Structure;
}

namespace RandomDock {

  class ADFOptimizer : public GlobalSearch::Optimizer
  {
    Q_OBJECT

   public:
    explicit ADFOptimizer(GlobalSearch::OptBase *parent,
                          const QString &filename = "");

    bool checkForSuccessfulOutput(GlobalSearch::Structure *s,
                                  bool *success);

  };

} // end namespace RandomDock

#endif
