/**********************************************************************
  MopacOptimizer - Tools to interface with Mopac

  Copyright (C) 2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifndef XO_MOPACOPTIMIZER_H
#define XO_MOPACOPTIMIZER_H

#include <xtalopt/optimizers/xtaloptoptimizer.h>

namespace XtalOpt {

  class MopacOptimizer : public XtalOptOptimizer
  {
    Q_OBJECT

   public:
    explicit MopacOptimizer(GlobalSearch::OptBase *parent,
                            const QString &filename = "");

  };

} // end namespace XtalOpt

#endif
