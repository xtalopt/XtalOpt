/************************************************************************
  OpenBabelOptimizer - Dummy optimizer interface for use with OpenBabelQI

  Copyright (C) 2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 *************************************************************************/

#ifndef OPENBABELOPTIMIZER_H
#define OPENBABELOPTIMIZER_H

#include <xtalopt/optimizers/xtaloptoptimizer.h>

namespace XtalOpt
{

class OpenBabelOptimizer : public XtalOptOptimizer
{
    Q_OBJECT
public:
  explicit OpenBabelOptimizer(GlobalSearch::OptBase *parent = 0,
                              const QString &filename = "");

  // These don't need to do anything
  bool checkIfOutputFileExists(GlobalSearch::Structure *s, bool *exists)
  {
    *exists = true;
    return true;
  }
  bool checkForSuccessfulOutput(GlobalSearch::Structure *s, bool *success)
  {
    *success = true;
    return true;
  }
  bool update(GlobalSearch::Structure *s) {return true;} // handled in QI
  // bool load(...)  still needs to work for save/resume
  int getNumberOfOptSteps() {return 1;}

};

}


#endif // OPENBABELOPTIMIZER_H
