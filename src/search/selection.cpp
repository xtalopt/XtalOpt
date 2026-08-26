/**********************************************************************
  selection - Parent-selection math implementation.

  Copyright (C) 2010-2011 by David C. Lonie
  Copyright (C) 2024 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <search/selection.h>

#include <cfloat>
#include <cmath>
#include <algorithm>
#include <limits>
#include <map>
#include <numeric>

#include <common/compatibility/platform_compat.h>
#include <common/constants.h>
#include <common/numericutils.h>

namespace Search {

namespace {

// A set of helper functions for Jensen-Fortin-Buzdalov (JFB) non-dominated sorting.
// In this calculation:
//   - Exact duplicates are merged before the recursion starts,
//   - the index lists always keep the dictionary order.
// So whenever a point comes before another one and is <= on every objective, it dominates it.
// P holds the unique points and rank their front indices.

// Check "a <= b" on objectives 0..k.
bool jfbLeq(const double* a, const double* b, int k)
{
  for (int j = 0; j <= k; ++j)
    if (a[j] > b[j])
      return false;
  return true;
}

// (objective value, front) pairs for the two-objective checks; both increase
//   over map, so the entry at or below a given one is the best front.
typedef std::map<double, int> JfbStaircase;

int jfbStaircaseBest(const JfbStaircase& stairs, double key)
{
  JfbStaircase::const_iterator it = stairs.upper_bound(key);
  if (it == stairs.begin())
    return -1;
  --it;
  return it->second;
}

void jfbStaircaseAdd(JfbStaircase& stairs, double key, int front)
{
  JfbStaircase::iterator it = stairs.lower_bound(key);
  if (it != stairs.begin()) {
    JfbStaircase::iterator below = it;
    --below;
    if (below->second >= front)
      return;
  }
  while (it != stairs.end() && it->second <= front)
    it = stairs.erase(it);
  stairs.insert(JfbStaircase::value_type(key, front));
}

// Assign fronts within S using objectives 0 and 1.
void jfbSweepA(const std::vector<const double*>& P, std::vector<int>& rank,
               const std::vector<int>& S)
{
  JfbStaircase stairs;
  for (size_t i = 0; i < S.size(); ++i) {
    const int p = S[i];
    const int best = jfbStaircaseBest(stairs, P[p][1]);
    if (best + 1 > rank[p])
      rank[p] = best + 1;
    jfbStaircaseAdd(stairs, P[p][1], rank[p]);
  }
}

// Raise the fronts of H for dominance from L using objectives 0 and 1.
void jfbSweepB(const std::vector<const double*>& P, std::vector<int>& rank,
               const std::vector<int>& L, const std::vector<int>& H)
{
  JfbStaircase stairs;
  size_t li = 0;
  for (size_t hi = 0; hi < H.size(); ++hi) {
    const int h = H[hi];
    while (li < L.size() && P[L[li]][0] <= P[h][0]) {
      jfbStaircaseAdd(stairs, P[L[li]][1], rank[L[li]]);
      ++li;
    }
    const int best = jfbStaircaseBest(stairs, P[h][1]);
    if (best + 1 > rank[h])
      rank[h] = best + 1;
  }
}

// Keep the members of S whose objective-k value is below (-1), at (0), or
//   above (+1) the pivot; the dictionary order is preserved.
std::vector<int> jfbPart(const std::vector<const double*>& P, const std::vector<int>& S,
                         int k, double pivot, int which)
{
  std::vector<int> part;
  part.reserve(S.size());
  for (size_t i = 0; i < S.size(); ++i) {
    const double v = P[S[i]][k];
    if ((which < 0 && v < pivot) || (which == 0 && v == pivot) || (which > 0 && v > pivot))
      part.push_back(S[i]);
  }
  return part;
}

// The median of the objective-k values over one or two index lists.
double jfbMedian(const std::vector<const double*>& P, const std::vector<int>& S1,
                 const std::vector<int>& S2, int k)
{
  std::vector<double> values;
  values.reserve(S1.size() + S2.size());
  for (size_t i = 0; i < S1.size(); ++i)
    values.push_back(P[S1[i]][k]);
  for (size_t i = 0; i < S2.size(); ++i)
    values.push_back(P[S2[i]][k]);
  std::nth_element(values.begin(), values.begin() + values.size() / 2, values.end());
  return values[values.size() / 2];
}

// Raise the fronts of H for dominance from L using objectives 0..k
//   (L is never changed here: its fronts must already be final).
void jfbHelperB(const std::vector<const double*>& P, std::vector<int>& rank,
                const std::vector<int>& L, const std::vector<int>& H, int k)
{
  if (L.empty() || H.empty())
    return;

  if (L.size() == 1 || H.size() == 1) {
    for (size_t li = 0; li < L.size(); ++li)
      for (size_t hi = 0; hi < H.size(); ++hi)
        if (jfbLeq(P[L[li]], P[H[hi]], k) && rank[L[li]] + 1 > rank[H[hi]])
          rank[H[hi]] = rank[L[li]] + 1;
    return;
  }

  if (k == 1) {
    jfbSweepB(P, rank, L, H);
    return;
  }

  // A single objective-k value everywhere: revert to the earlier objectives.
  double minv = P[L[0]][k], maxv = minv;
  for (size_t i = 0; i < L.size(); ++i) {
    const double v = P[L[i]][k];
    if (v < minv) minv = v;
    if (v > maxv) maxv = v;
  }
  for (size_t i = 0; i < H.size(); ++i) {
    const double v = P[H[i]][k];
    if (v < minv) minv = v;
    if (v > maxv) maxv = v;
  }
  if (minv == maxv) {
    jfbHelperB(P, rank, L, H, k - 1);
    return;
  }

  const double med = jfbMedian(P, L, H, k);
  const std::vector<int> lowL = jfbPart(P, L, k, med, -1);
  const std::vector<int> midL = jfbPart(P, L, k, med, 0);
  const std::vector<int> uppL = jfbPart(P, L, k, med, +1);
  const std::vector<int> lowH = jfbPart(P, H, k, med, -1);
  const std::vector<int> midH = jfbPart(P, H, k, med, 0);
  const std::vector<int> uppH = jfbPart(P, H, k, med, +1);

  // Same-side pairs keep objective k; crossing pairs don't need it anymore.
  jfbHelperB(P, rank, lowL, lowH, k);
  jfbHelperB(P, rank, lowL, midH, k - 1);
  jfbHelperB(P, rank, lowL, uppH, k - 1);
  jfbHelperB(P, rank, midL, midH, k - 1);
  jfbHelperB(P, rank, midL, uppH, k - 1);
  jfbHelperB(P, rank, uppL, uppH, k);
}

// Assign fronts within S using objectives 0..k.
void jfbHelperA(const std::vector<const double*>& P, std::vector<int>& rank,
                const std::vector<int>& S, int k)
{
  if (S.size() < 2)
    return;

  if (S.size() == 2) {
    if (jfbLeq(P[S[0]], P[S[1]], k) && rank[S[0]] + 1 > rank[S[1]])
      rank[S[1]] = rank[S[0]] + 1;
    return;
  }

  if (k == 0) {
    // One objective left: in this order every point dominates the later ones.
    int best = rank[S[0]];
    for (size_t i = 1; i < S.size(); ++i) {
      if (best + 1 > rank[S[i]])
        rank[S[i]] = best + 1;
      if (rank[S[i]] > best)
        best = rank[S[i]];
    }
    return;
  }

  if (k == 1) {
    jfbSweepA(P, rank, S);
    return;
  }

  double minv = P[S[0]][k], maxv = minv;
  for (size_t i = 1; i < S.size(); ++i) {
    const double v = P[S[i]][k];
    if (v < minv) minv = v;
    if (v > maxv) maxv = v;
  }
  if (minv == maxv) {
    jfbHelperA(P, rank, S, k - 1);
    return;
  }

  const double med = jfbMedian(P, S, std::vector<int>(), k);

  const std::vector<int> lower = jfbPart(P, S, k, med, -1);
  const std::vector<int> onPivot = jfbPart(P, S, k, med, 0);
  const std::vector<int> upper = jfbPart(P, S, k, med, +1);

  // The call order matters: a group's fronts are resolved before they are pushed onto a later group.
  jfbHelperA(P, rank, lower, k);
  jfbHelperB(P, rank, lower, onPivot, k - 1);
  jfbHelperA(P, rank, onPivot, k - 1);
  jfbHelperB(P, rank, lower, upper, k - 1);
  jfbHelperB(P, rank, onPivot, upper, k - 1);
  jfbHelperA(P, rank, upper, k);
}

} // namespace



/**
 * Check whether objective vector @p x dominates @p y for minimization.
 */
bool doesDominate(const double* x, const double* y, int m) noexcept
{
  // This function checks if a solution structure dominates another one
  bool any_lt = false;
  for (int k = 0; k < m; ++k) {
    double xi = x[k], yi = y[k];
    if (xi > yi) return false;
    any_lt |= (xi < yi);
  }
  return any_lt;
}

//=========================================================================================

std::vector<double> frontCrowdingDistance(const std::vector<std::vector<double>>& front)
{
  // This function returns raw crowding distances for points in a given front

  const size_t pnts_frnt = front.size();
  if (pnts_frnt < 2) return std::vector<double>(pnts_frnt, 0.0);

  const size_t data_pnts = front[0].size(); // number of objective data per point

  std::vector<double> distances(pnts_frnt, 0.0);
  std::vector<size_t> indices(pnts_frnt);
  std::iota(indices.begin(), indices.end(), 0);

  for (size_t i = 0; i < data_pnts; ++i) {
    // Sort based on the i-th objective
    std::sort(indices.begin(), indices.end(), [&front, i](size_t a, size_t b) {
        return front[a][i] < front[b][i];
        });

    distances[indices[0]] = distances[indices[pnts_frnt - 1]] = std::numeric_limits<double>::infinity();
    double range = front[indices[pnts_frnt - 1]][i] - front[indices[0]][i];

    if (range > 0) {
      for (size_t j = 1; j < pnts_frnt - 1; ++j) {
        distances[indices[j]] += (front[indices[j + 1]][i] - front[indices[j - 1]][i]) / range;
      }
    }
  }

  return distances;
}

//=========================================================================================

std::vector<double> scaledCrowdingDistances(const std::vector<std::vector<double>>& points,
                                            const std::vector<std::vector<int>>& fronts,
                                            std::vector<double>& raw_dists)
{
  // This function returns a list for "raw distances" and "scaled distances" (to [0.1, 1])
  //   for all data points.
  // It goes over all fronts one by one, and calculates the raw distances first; then
  //   scales them. If all distances of a given front are inf/nan or zero, they are
  //   set to 1.0. And if there are some inf/nan, they are set to maximum distance in the front.

  std::vector<double> scl_dists(points.size(), 1.0);

  int ndim = points[0].size();
  for (size_t i = 0; i < fronts.size(); i++) {
    size_t npnt = fronts[i].size();
    std::vector<std::vector<double>> front_points(npnt, std::vector<double>(ndim));
    for (size_t j = 0; j < npnt; j++) {
      for (int k = 0; k < ndim; k++)
        front_points[j][k] = points[fronts[i][j]][k];
    }

    std::vector<double> dists = frontCrowdingDistance(front_points);

    for (size_t j = 0; j < npnt; j++)
      raw_dists[fronts[i][j]] = dists[j];

    // Check if all distances of this front are inf/nan or zero
    bool allnan = true;
    bool allzer = true;
    for (size_t j = 0; j < npnt; j++)
      if (!GS_ISINF(dists[j]) && !GS_ISNAN(dists[j])) {
        allnan = false;
        if (dists[j] > 0.0)
          allzer = false;
      }
    // If all distances of this front are inf/nan or zero,
    //   just set them to the default 1.0
    if (allnan || allzer) {
      for (size_t j = 0; j < npnt; j++)
        dists[j] = 1.0;
    }

    // Find the max/min values of non-inf/nan distances for this front
    double max_dist = -DBL_MAX;
    double min_dist =  DBL_MAX;
    for (size_t j = 0; j < npnt; j++) {
      if (!GS_ISINF(dists[j]) && !GS_ISNAN(dists[j])) {
        if (dists[j] > max_dist)
          max_dist = dists[j];
        if (dists[j] < min_dist)
          min_dist = dists[j];
      }
    }

    // Scale distances to [0.1, 1]; while setting inf/nan values to max distance
    for (size_t j = 0; j < npnt; j++) {
      if (GS_ISINF(dists[j]) || GS_ISNAN(dists[j]))
        dists[j] = max_dist;
      if (Common::neq(max_dist, min_dist, ZERO06)) {
        scl_dists[fronts[i][j]] = 0.1 + 0.9 * (dists[j] - min_dist) / (max_dist - min_dist);
      } else {
        scl_dists[fronts[i][j]] = dists[j] / max_dist;
      }
    }
  }

  return scl_dists;
}

//=========================================================================================

std::vector<std::vector<int>> nonDominatedSorting(const std::vector<std::vector<double>>& points)
{
  // The JFB divide-and-conquer non-dominated sorting.
  const int n = static_cast<int>(points.size());
  if (n == 0)
    return {};
  const int m = static_cast<int>(points[0].size());

  // Sort the point indices in dictionary order: a point can only be
  //   dominated by the ones before it.
  std::vector<int> order(n);
  std::iota(order.begin(), order.end(), 0);
  std::sort(order.begin(), order.end(),
            [&points](int a, int b) {
              const std::vector<double>& x = points[a];
              const std::vector<double>& y = points[b];
              for (size_t j = 0; j < x.size(); ++j) {
                if (x[j] < y[j]) return true;
                if (x[j] > y[j]) return false;
              }
              return a < b;
            });

  // Merge exact duplicates: equal points never dominate each other, so a
  //   whole group shares the front of its first member.
  std::vector<const double*> P;
  std::vector<std::vector<int>> group;
  P.reserve(n);
  group.reserve(n);
  for (int i = 0; i < n; ++i) {
    const double* point = points[order[i]].data();
    bool same = !P.empty();
    if (same) {
      for (int j = 0; j < m; ++j)
        if (P.back()[j] != point[j]) {
          same = false;
          break;
        }
    }
    if (same) {
      group.back().push_back(order[i]);
    } else {
      P.push_back(point);
      group.push_back(std::vector<int>(1, order[i]));
    }
  }

  const int uniqueCount = static_cast<int>(P.size());
  std::vector<int> rank(uniqueCount, 0);
  std::vector<int> S(uniqueCount);
  std::iota(S.begin(), S.end(), 0);
  jfbHelperA(P, rank, S, m - 1);

  int numFronts = 0;
  for (int i = 0; i < uniqueCount; ++i)
    if (rank[i] + 1 > numFronts)
      numFronts = rank[i] + 1;

  std::vector<std::vector<int>> fronts(numFronts);
  for (int i = 0; i < uniqueCount; ++i)
    fronts[rank[i]].insert(fronts[rank[i]].end(), group[i].begin(), group[i].end());
  return fronts;
}

//=========================================================================================

std::vector<std::vector<int>> nonDominatedSortingDeb(const std::vector<std::vector<double>>& points)
{
  // Legacy NSGA-II (Deb 2002) non-dominated sroting: retired as of v15 of the code!
  // This function performs non-dominated sorting for a set of objectives, all to be minimized,
  //   and returns "fronts" that contains vectors of point indices which belong to each rank.
  const size_t n = points.size();
  if (n == 0) return {};

  const int m = static_cast<int>(points[0].size());
  std::vector<int> domCount(n, 0);
  std::vector<std::vector<int>> S(n);
  for (auto& v : S) v.reserve(8);

  // Pairwise checks once (i<j), update both sides
  for (size_t i = 0; i < n; ++i) {
    const double* xi = points[i].data();
    for (size_t j = i + 1; j < n; ++j) {
      const double* xj = points[j].data();
      if (doesDominate(xi, xj, m)) {
        S[i].push_back(static_cast<int>(j));
        ++domCount[j];
      } else if (doesDominate(xj, xi, m)) {
        S[j].push_back(static_cast<int>(i));
        ++domCount[i];
      }
    }
  }

  // Build fronts
  std::vector<std::vector<int>> fronts;
  fronts.emplace_back();
  fronts.back().reserve(n);
  for (int i = 0; i < (int)n; ++i)
    if (domCount[i] == 0) fronts.back().push_back(i);

  while (!fronts.back().empty()) {
    const auto& F = fronts.back();
    std::vector<int> next;
    for (int p : F) {
      for (int q : S[p]) {
        if (--domCount[q] == 0) next.push_back(q);
      }
    }
    if (next.empty()) break;
    fronts.emplace_back(std::move(next));
  }
  return fronts;
}

//=========================================================================================

std::vector<double> paretoProbs(const std::vector<std::vector<double>>& points, bool crwdDist,
                                std::vector<int>& pntfrnts, std::vector<double>& scldists,
                                std::vector<double>& rawprobs, std::vector<double>& rawdists)
{
// This function returns the Pareto front (rank) of all structures, and the scalar Pareto-based
//   fitness measure obtained from ranks (and -optionally- crowding distances).
  // It is assumed that:
  //   (1) objective are all to be minimized,
  //   (2) the desired precision is applied to objective values.
  //
  // The workflow starts by non-dominated sorting. The result is a 2D vector "fronts"
  //   where rows are front indices (0...n) and each row has column indices (0...m) such
  //   that [n][m] is the structure index (0...s) belonging to the front "n".
  // Using the ranks, the raw scalar probs are calculated.
  // Then, if instructed, the crowding distances are calculated and scaled, then applied
  //   to the final probabilities returned from this function.
  //
  // Since objectives are of minimization type, the front with lowest index is the
  //   global non-dominated list.

  // Initialize some variables
  int ndat = points.size();
  std::vector<double> finprobs(ndat, 0.0);

  // Perform non-dominated sorting
  std::vector<std::vector<int>> fronts = nonDominatedSorting(points);
  int numfrnts = fronts.size();

  // Sanity checks: make sure "fronts" is not empty and includes all structures
  if (numfrnts == 0)
    return finprobs;

  int strcount = 0;
  for (int i = 0; i < numfrnts; i++)
    strcount += fronts[i].size();

  if (strcount != ndat)
    return finprobs;

  // Assign the ranks and raw probs (and set the finprobs equal to rawprobs for now)
  for (int i = 0; i < numfrnts; i++) {
    for (int j = 0; j < static_cast<int>(fronts[i].size()); j++) {
      int indx = fronts[i][j];
      pntfrnts[indx] = i;
      finprobs[indx] = rawprobs[indx] = (double)(numfrnts - i) / numfrnts;
    }
  }

  // Crowding distances: if needed, we calculate and apply them to the finprobs
  if (crwdDist) {
    scldists = scaledCrowdingDistances(points, fronts, rawdists);
    for (int i = 0; i < numfrnts; i++) {
      for (int j = 0; j < static_cast<int>(fronts[i].size()); j++) {
        int indx = fronts[i][j];
        finprobs[indx] -= (1.0 - scldists[indx]) / numfrnts;
      }
    }
  }

  return finprobs;
}

//=========================================================================================


std::vector<double> scalarProbs(const std::vector<std::vector<double>>& points,
                                const std::vector<double>& weights)
{
  // This function returns the scalar fitness measure using a set of points (i.e., objective
  //   values) and their corresponding weight.
  // It is assumed that:
  //   (1) objective values are already scaled to [0,1],
  //   (2) objective are all to be minimized,
  //   (3) weights are normalized to 1.0,
  //   (4) the desired precision is applied to objective values.
  // With all objectives being "minimizable" and all weights and objective "normalized to 1",
  //   the fitness becomes: (1.0 - sum of objvalue*weight contributions for all objectives).

  // Initialize some variables
  int ndat = points.size();
  int nobj = weights.size();

  // Calculate probabilities
  std::vector<double> finprobs;

  for(int i = 0; i < ndat; i++) {
    double contrib = 0.0;
    for (int j = 0; j < nobj; j++) {
      contrib += (points[i][j] * weights[j]);
    }
    finprobs.push_back(1.0 - contrib);
  }

  return finprobs;
}


} // namespace Search
