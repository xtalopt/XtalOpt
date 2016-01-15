/**********************************************************************
  elemInfoDatabase.h - Contains the database information needed in elemInfo.h
                       Contains atomic symbol and atomic radii information

  Copyright (C) 2015 - 2016 by Patrick S. Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

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
