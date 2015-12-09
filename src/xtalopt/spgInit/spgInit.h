
#ifndef SPG_INIT_H
#define SPG_INIT_H

#include <Eigen/LU>
#include <vector>
#include <tuple>
#include <utility>

typedef struct {
  uint atomicNum;
  double x;
  double y;
  double z;
} atomStruct;

typedef std::tuple<char, int, std::string> wyckInfo;
typedef std::vector<wyckInfo> wyckoffPositions;

class SpgInit {
 public:
  static const wyckoffPositions& getWyckoffPositions(uint spg);
  static std::vector<atomStruct> generateInitWyckoffs(uint spg, std::vector<uint> atomTypes);
  static inline std::vector<std::pair<uint, bool> > getMultiplicityVector(wyckoffPositions& pos);
  static inline bool containsUniquePosition(wyckInfo& info);
  static inline std::vector<uint> getNumOfEachType(std::vector<uint> atomTypes);
  static bool isSpgPossible(uint spg, std::vector<uint> atomTypes);
};

#endif
