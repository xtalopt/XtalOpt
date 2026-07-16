/**********************************************************************
  selection - Parent-selection math: fitness scalarization, Pareto
              sorting, crowding distances, and selection probabilities.

  Copyright (C) 2010-2011 by David C. Lonie
  Copyright (C) 2024 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef SEARCH_SELECTION_H
#define SEARCH_SELECTION_H

#include <vector>

namespace Search {

/** @return Whether solution @p x dominates @p y over @p m objectives. */
bool doesDominate(const double* x, const double* y, int m) noexcept;

/** @return Raw crowding distances for the points of one Pareto front. */
std::vector<double> frontCrowdingDistance(const std::vector<std::vector<double>>& front);

/**
 * @return Scaled crowding distances for @p points grouped by @p fronts;
 * @p raw_dists receives the unscaled distances.
 */
std::vector<double> scaledCrowdingDistances(const std::vector<std::vector<double>>& points,
  const std::vector<std::vector<int>>& fronts, std::vector<double>& raw_dists);

/** @return Pareto fronts (lists of point indices), best front first. */
std::vector<std::vector<int>> nonDominatedSorting(const std::vector<std::vector<double>>& points);

/** Legacy NSGA-II (Deb 2002) non-dominated sroting */
std::vector<std::vector<int>> nonDominatedSortingDeb(const std::vector<std::vector<double>>& points);

/**
 * @return Pareto-based selection probabilities for @p points.
 * @p pntfrnts receives each point's front index, @p scldists the scaled
 * crowding distances, @p rawprobs/@p rawdists the unscaled values.
 */
std::vector<double> paretoProbs(const std::vector<std::vector<double>>& points, bool crwdDist,
  std::vector<int>& pntfrnts, std::vector<double>& scldists,
  std::vector<double>& rawprobs, std::vector<double>& rawdists);

/** @return Selection probabilities from scalarized objective values. */
std::vector<double> scalarProbs(const std::vector<std::vector<double>>& points,
  const std::vector<double>& weights);

} // namespace Search

#endif // SEARCH_SELECTION_H
