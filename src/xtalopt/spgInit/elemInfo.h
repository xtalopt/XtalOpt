/**********************************************************************
  elemInfoDatabase.h - Contains the database information needed in elemInfo.h
                       Contains atomic symbol and atomic radii information

  Copyright (C) 2016 by Patrick S. Avery

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifndef ELEM_INFO_H
#define ELEM_INFO_H

#include "elemInfoDatabase.h"

#include <iostream>

namespace ElemInfo {

  std::string getAtomicSymbol(uint atomicNum)
  {
    if (atomicNum == 0 || atomicNum > 117) {
      std::cout << "Error: Invalid atomicNum, " << atomicNum << ", was entered in "
           << __FUNCTION__ << "!\n";
      return 0;
    }
    return _atomicSymbols.at(atomicNum);
  }

  uint getAtomicNum(std::string symbol)
  {
    for (uint i = 0; i < _atomicSymbols.size(); i++) {
      if (_atomicSymbols.at(i) == symbol) return i;
    }

    std::cout << "Error: Invalid symbol, " << symbol << ", was entered into "
         << __FUNCTION__ << "!\n";
    return 0;
  }

  double getVdwRadius(uint atomicNum)
  {
    if (atomicNum == 0 || atomicNum > 117) {
      std::cout << "Error: Invalid atomicNum, " << atomicNum << ", was entered in "
           << __FUNCTION__ << "!\n";
      return 0;
    }
    return _vdwRadii.at(atomicNum);
  }

  double getCovalentRadius(uint atomicNum)
  {
    if (atomicNum == 0 || atomicNum > 117) {
      std::cout << "Error: Invalid atomicNum, " << atomicNum << ", was entered in "
           << __FUNCTION__ << "!\n";
      return 0;
    }
    return _covalentRadii.at(atomicNum);
  }

  // We allow the user to set a radius here
  // We will set both radii since we know the user will only be using one of them,
  // but we don't know which...
  void setRadius(uint atomicNum, double newRadius)
  {
    if (atomicNum == 0 || atomicNum > 117) {
      std::cout << "Error: Invalid atomicNum, " << atomicNum << ", was entered in "
           << __FUNCTION__ << "!\n";
      return;
    }
    _covalentRadii[atomicNum] = newRadius;
    _vdwRadii[atomicNum] = newRadius;
  }

} // namespace ElemInfo

#endif
