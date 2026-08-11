/**********************************************************************
  chull - Interface to Qhull library for computing the convex hull.

  Copyright (C) 2024 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <common/chull.h>

#include <common/output.h>
#include <common/constants.h>
#include <common/numericutils.h>


#include "libqhullcpp/Qhull.h"
#include "libqhullcpp/QhullFacetList.h"
#include "libqhullcpp/QhullError.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace Common {

bool distAboveHull(const std::vector<double>& input_data_ref, int input_num, int input_dim,
                   std::vector<double>& above_hull)
{
  using namespace orgQhull;
  // In this function, we calculate and return the distance above hull for input data.
  // We work on a local copy since the data is normalized / converted internally.
  std::vector<double> input_data(input_data_ref);
  //
  // input_data holds the data for "input_num" points. Each point is a
  //   "composition" over "r" elements plus an "energy", so each point is
  //   "input_dim = r + 1" long and the whole vector is input_num * input_dim.
  //   The output "above_hull" then has one entry per point (length input_num).
  // We assume the proper reference points (the elemental entries) are in the data.
  //
  // The overall workflow:
  //   1) normalize the compositions and energies, and find the elemental references,
  //   2) check we actually have those references, and use them to turn input
  //      energies into formation energies,
  //   3) drop the first composition coordinate from each entry,
  //   4) compute the convex hull,
  //   5) return the distance above the hull for every point.
  //
  // We overwrite input_data as we go, but that's only our local copy - none of
  //   those changes leak back to the caller.

  //===== Check the input and size the output
  if (input_num < 1 || input_dim < 2 ||
      static_cast<int>(input_data.size()) != input_num * input_dim) {
    Common::error("Hull calculation received inconsistent input dimensions.");
    return false;
  }
  above_hull.assign(input_num, 0.0);

  //===== Process input data
  //minimum elemental enthalpy per atom values
  std::vector<double> min_ele(input_dim - 1, PINF);
  //normalize the entries (just in case); and find elemental references
  for(int i = 0; i < input_num; i++) {
    double tot_com = 0.0;
    for(int j = 0; j < input_dim - 1; j++)
      tot_com += input_data[i * input_dim + j];
    if (std::fabs(tot_com) < ZERO06) {
      Common::error("Hull calculation found an entry with zero composition.");
      return false;
    }
    for(int j = 0; j < input_dim; j++)
      input_data[i * input_dim + j] /= tot_com;
    for(int j = 0; j < input_dim - 1; j++)
      if (Common::fuzzyCompare(input_data[i * input_dim + j], 1.0, ZERO06))
        if(input_data[i * input_dim + input_dim - 1] < min_ele[j])
          min_ele[j] = input_data[i * input_dim + input_dim - 1];
  }

  //a sanity check: did we find all elemental references?
  for(int i = 0; i < input_dim - 1; i++)
    if (min_ele[i] == PINF) {
      Common::error("Hull calculation failed to find all elemental references!");
      return false;
    }

  //convert input "enthalpy/atom" to "formation energy/atom"
  for(int i = 0; i < input_num; i++) {
    double enth = 0.0;
    for(int j = 0; j < input_dim - 1; j++)
      enth += (input_data[i * input_dim + j] * min_ele[j]);
    input_data[i * input_dim + input_dim - 1] -= enth;
  }

  //===== If the input is for an elemental system; we're done!
  if (input_dim == 2) {
    for (int i = 0; i < input_num; i++) {
      above_hull[i] = input_data[i * input_dim + 1];
    }
    return true;
  }

  //===== Prepare hull input
  //dimension of the hull: we will remove first column of input data
  int hll_ndim = input_dim - 1;
  //total number of points
  int hll_npnt = input_num;
  //"reduced" hull input vector
  std::vector<double> hll_inpt;
  hll_inpt.reserve(hll_npnt * hll_ndim);
  //initialize reduced hull input vector
  for(int i = 0; i < hll_npnt; i++)
    for(int j = 0; j < hll_ndim; j++)
      hll_inpt.push_back(input_data[i* (hll_ndim + 1) + (j + 1)]);

  //===== Initialize Qhull and run convex hull algorithm
  //we report any problem ourselves, so keep the library quiet
  std::ostringstream quiet;
  Qhull qhull;
  qhull.setErrorStream(&quiet);
  qhull.setOutputStream(&quiet);
  try {
    qhull.runQhull("i", hll_ndim, hll_npnt, hll_inpt.data(), "Qt");
  } catch(QhullError &e) {
    Common::error(QString("Hull calculations had 'Qhull' error output: %1 %2")
                   .arg(e.what())
                   .arg(QString::fromStdString(quiet.str())));
    return false;
  }

  //===== Collect hull info for calculating the distances
  QhullFacetList facets = qhull.facetList();
  //number of facets
  int hll_npln = facets.size();
  //hyperplane equations
  std::vector< std::vector<double> > hll_eqns;
  std::vector<double> hll_ofst;
  for (const QhullFacet& facet : facets) {
    QhullHyperplane hp = facet.hyperplane();
    std::vector<double> tmpeqn(hp.coordinates(), hp.coordinates() + hll_ndim);
    hll_eqns.push_back(tmpeqn);
    hll_ofst.push_back(facet.hyperplane().offset());
  }

  //===== Sanity check!
  if (hll_eqns.empty()) {
    Common::error("Hull calculations produced no facets!");
    return false;
  }
  if (hll_ndim != static_cast<int>(hll_eqns[0].size())) {
    Common::error("Hull calculations had issue with dimensions!");
    return false;
  }

  //===== Find distance of the points from the hull
  //normal vector towards an "imaginary" facet on which the projection
  //  of a point has higher energy than the point itself, i.e., a facet
  //  located "above" the point.
  std::vector<double> upward(hll_ndim, 0.0);
  upward.back() = 1.0;
  //find distances of the point from all facets that are "below" it
  for(int i= 0; i< hll_npnt; i++) {
    std::vector<double> distances;
    for(int j = 0; j < hll_npln; j++) {
      //check if the facet is "below" the point and "proper"
      double dotprod = 0.0;
      for (int k = 0; k < hll_ndim; k++)
        dotprod += hll_eqns[j][k] * upward[k];
      if(dotprod > ZERO06 || std::fabs(hll_eqns[j].back()) < ZERO06)
        continue;
      //find the "energy coordinate" of the corresponding point on the facet
      double ener_coor = 0.0;
      for (int k = 0; k < hll_ndim - 1; k++)
        ener_coor -= hll_inpt[i * hll_ndim + k] * hll_eqns[j][k];
      ener_coor -= hll_ofst[j];
      ener_coor /= hll_eqns[j][hll_ndim-1];
      //distance between the point and its projection: energy difference!
      double dist = hll_inpt[i * hll_ndim + hll_ndim - 1] - ener_coor;
      //adjust for numerical precision
      if (std::fabs(dist) < ZERO06)
        dist = 0.0;
      //save the distance
      distances.push_back(dist);
    }
    //find the shortest distance among the distances from lower facets
    std::sort(distances.begin(), distances.end());
    //sanity check (we shouldn't have any negative distances at this point!)
    if (distances.size() == 0 || distances[0] < 0.0) {
      Common::error(QString("Hull calculation failed to find distance for point %1").arg(i+1));
      return false;
    }
    //save the calculated distance above hull
    above_hull[i] = distances[0];
  }

  return true;
}

bool convexHullPlanes(const std::vector<double>& points, int numPoints, int dim,
                      std::vector<double>& normals, std::vector<double>& offsets)
{
  using namespace orgQhull;
  normals.clear();
  offsets.clear();

  // A hull in "dim" dimensions needs at least dim+1 points.
  if (dim < 1 || numPoints < dim + 1 ||
      static_cast<int>(points.size()) < numPoints * dim)
    return false;

  // Return false for points that cannot make a hull.
  std::ostringstream quiet;
  Qhull qhull;
  qhull.setErrorStream(&quiet);
  qhull.setOutputStream(&quiet);
  try {
    qhull.runQhull("", dim, numPoints, points.data(), "Qt");
  } catch (QhullError&) {
    return false;
  }

  const QhullFacetList facets = qhull.facetList();
  if (facets.size() == 0)
    return false;

  normals.reserve(static_cast<size_t>(facets.size()) * dim);
  offsets.reserve(facets.size());
  for (const QhullFacet& facet : facets) {
    QhullHyperplane hp = facet.hyperplane();
    const double* coordinates = hp.coordinates();
    for (int k = 0; k < dim; ++k)
      normals.push_back(coordinates[k]);
    offsets.push_back(hp.offset());
  }
  return true;
}

bool convexHullVolume(const std::vector<double>& points, int numPoints, int dim,
                      double& volume)
{
  using namespace orgQhull;
  volume = 0.0;

  // A hull in "dim" dimensions needs at least dim+1 points.
  if (dim < 1 || numPoints < dim + 1 ||
      static_cast<int>(points.size()) < numPoints * dim)
    return false;

  // Return false for points that cannot make a hull.
  std::ostringstream quiet;
  Qhull qhull;
  qhull.setErrorStream(&quiet);
  qhull.setOutputStream(&quiet);
  try {
    qhull.runQhull("", dim, numPoints, points.data(), "Qt");
    volume = qhull.volume();
  } catch (QhullError&) {
    volume = 0.0;
    return false;
  }

  return true;
}

} // namespace Common
