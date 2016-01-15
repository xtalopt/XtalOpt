/**********************************************************************
  wyckPosTrackingInfo.h - Class for tracking Wyckoff position
                          information. It is used in SpgInitCombinatoric.cpp
                          for findAllCombinations(). It keeps track of
                          which Wyckoff positions to investigate and
                          other information about them.

  Copyright (C) 2015 - 2016 by Patrick S. Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

 ***********************************************************************/

#ifndef WYCKPOS_TRACKING_INFO_H
#define WYCKPOS_TRACKING_INFO_H

#include <xtalopt/spgInit/spgInit.h>

// In here, we keep Wyckoff positions that have the same uniqueness and
// multiplicity. For now, they can only be non-unique
// The 'wyckPos' is the wyckoff position
// The first 'bool' is whether to keep using this position or not
// The second 'bool' is whether it is unique or not
// The uint is the number of times it's been used
class WyckPosTrackingInfo {
 public:
  WyckPosTrackingInfo(std::vector<wyckPos> pos) :
  keepUsing(true),
  unique(SpgInit::containsUniquePosition(pos.at(0))),
  numTimesUsed(0),
  multiplicity(SpgInit::getMultiplicity(pos.at(0))),
  positions(pos)
{
  // We must have a size greater than 0
  assert(positions.size() > 0);
};

  // This is to see if we should keep checking this one duing
  bool keepUsing;
  bool unique;
  uint numTimesUsed;
  uint multiplicity;

  wyckPos getRandomWyckPos() const
  {
    return positions.at(rand() % positions.size());
  };

  const wyckPos& getWyckPosAt(uint i) const {return positions.at(i);};
  size_t getNumPositions() const {return positions.size();};
 private:
  // This has more than one if there are identical positions
  std::vector<wyckPos> positions;
};

#endif
