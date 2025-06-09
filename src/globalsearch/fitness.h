/**********************************************************************
  fitness.h - A collection of functions for calculating fitness.

  Copyright (C) 2024 by Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

 ***********************************************************************/

#ifndef FITNESS_H
#define FITNESS_H

#include <cfloat>
#include <cmath>
#include <iostream>
#include <vector>
#include <algorithm>
#include <limits>
#include <numeric>

#include <globalsearch/constants.h>

//=========================================================================================

bool doesDominate(const std::vector<double>& x, const std::vector<double>& y)
{
  // This function checks if a solution candidate dominates another one
  bool all_le = true;
  bool any_lt = false;
  for (int i = 0; i < x.size(); ++i) {
    if (x[i] > y[i]) {
      all_le = false;
    }
    if (x[i] < y[i]) {
      any_lt = true;
    }
  }
  return all_le && any_lt;
}

//=========================================================================================

std::vector<double> frontCrowdingDistance(const std::vector<std::vector<double>>& front)
{
  // This function returns raw crowding distances for points in a given front

  auto pnts_frnt = front.size();
  if (pnts_frnt < 2) return std::vector<double>(pnts_frnt, 0.0);

  auto data_pnts = front[0].size(); // number of objective data per point

  std::vector<double> distances(pnts_frnt, 0.0);
  std::vector<int> indices(pnts_frnt);
  std::iota(indices.begin(), indices.end(), 0);

  for (int i = 0; i < data_pnts; ++i) {
    // Sort based on the i-th objective
    std::sort(indices.begin(), indices.end(), [&front, i](int a, int b) {
        return front[a][i] < front[b][i];
        });

    distances[indices[0]] = distances[indices[pnts_frnt - 1]] = std::numeric_limits<double>::infinity();
    double range = front[indices[pnts_frnt - 1]][i] - front[indices[0]][i];

    if (range > 0) {
      for (int j = 1; j < pnts_frnt - 1; ++j) {
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
  for (int i = 0; i < fronts.size(); i++) {
    int npnt = fronts[i].size();
    std::vector<std::vector<double>> front_points(npnt, std::vector<double>(ndim));
    for (int j = 0; j < npnt; j++) {
      for (int k = 0; k < ndim; k++)
        front_points[j][k] = points[fronts[i][j]][k];
    }

    std::vector<double> dists = frontCrowdingDistance(front_points);

    for (int j = 0; j < npnt; j++)
      raw_dists[fronts[i][j]] = dists[j];

    // Check if all distances of this front are inf/nan or zero
    bool allnan = true;
    bool allzer = true;
    for (int j = 0; j < npnt; j++)
      if (!std::isinf(dists[j]) && !std::isnan(dists[j])) {
        allnan = false;
        if (dists[j] > 0.0)
          allzer = false;
      }
    // If all distances of this front are inf/nan or zero,
    //   just set them to the default 1.0
    if (allnan || allzer) {
      for (int j = 0; j < npnt; j++)
        dists[j] = 1.0;
    }

    // Find the max/min values of non-inf/nan distances for this front
    double max_dist = -DBL_MAX;
    double min_dist =  DBL_MAX;
    for (int j = 0; j < npnt; j++) {
      if (!std::isinf(dists[j]) && !std::isnan(dists[j])) {
        if (dists[j] > max_dist)
          max_dist = dists[j];
        if (dists[j] < min_dist)
          min_dist = dists[j];
      }
    }

    // Scale distances to [0.1, 1]; while setting inf/nan values to max distance
    for (int j = 0; j < npnt; j++) {
      if (std::isinf(dists[j]) || std::isnan(dists[j]))
        dists[j] = max_dist;
      if (fabs(max_dist - min_dist) > ZERO6) {
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
  // This function performs non-dominated sorting for a set of objectives, all to be minimized,
  //   and returns "fronts" that contains vectors of point indices which belong to each rank.

  int ndat = points.size();

  std::vector<std::vector<int>> fronts;
  std::vector<int> wrk_ind(ndat);
  std::iota(wrk_ind.begin(), wrk_ind.end(), 0);

  std::vector<std::vector<double>> wrk_arr = points;
  while (!wrk_arr.empty()) {
    int s = wrk_arr.size();
    std::vector<int> dom(s, 0);

    // Compute domination matrix
    for (int i = 0; i < s; ++i)
      for (int j = 0; j < s; ++j)
        if (doesDominate(wrk_arr[i], wrk_arr[j]))
          dom[j]++;

    // Select non-dominated ones
    std::vector<int> sel;
    for (int i = 0; i < s; ++i)
      if (dom[i] == 0)
        sel.push_back(i);

    // Save the indices of the selected ones
    std::vector<int> front;
    for (int i : sel)
      front.push_back(wrk_ind[i]);
    fronts.push_back(front);

    // Remove selected points from wrk_arr and wrk_ind
    std::vector<std::vector<double>> new_arr;
    std::vector<int> new_ind;
    for (int i = 0; i < s; ++i)
      if (std::find(sel.begin(), sel.end(), i) == sel.end()) {
        new_arr.push_back(wrk_arr[i]);
        new_ind.push_back(wrk_ind[i]);
      }
    wrk_arr = new_arr;
    wrk_ind = new_ind;
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
    for (int j = 0; j < fronts[i].size(); j++) {
      int indx = fronts[i][j];
      pntfrnts[indx] = i;
      finprobs[indx] = rawprobs[indx] = (double)(numfrnts - i) / numfrnts;
    }
  }

  // Crowding distances: if needed, we calculate and apply them to the finprobs
  if (crwdDist) {
    scldists = scaledCrowdingDistances(points, fronts, rawdists);
    for (int i = 0; i < numfrnts; i++) {
      for (int j = 0; j < fronts[i].size(); j++) {
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

#endif // FITNESS_H
