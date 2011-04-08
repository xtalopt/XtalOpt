/**********************************************************************
  PWscfOptimizer - Tools to interface with PWscf

  Copyright (C) 2009-2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifndef PWSCFOPTIMIZER_H
#define PWSCFOPTIMIZER_H

#include <xtalopt/optimizers/xtaloptoptimizer.h>

#include <QtCore/QObject>

namespace GlobalSearch {
  class OptBase;
}

namespace XtalOpt {
  class PWscfOptimizer : public XtalOptOptimizer
  {
    Q_OBJECT

   public:
    PWscfOptimizer(GlobalSearch::OptBase *parent, const QString &filename = "");

  };

} // end namespace XtalOpt

#endif
