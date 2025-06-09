/**********************************************************************
  chull.h - Interface to Qhull library for computing the convex hull.

  Copyright (C) 2024 by Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

 ***********************************************************************/

#ifndef CHULL_H
#define CHULL_H

#include <QDebug>

#include "libqhullcpp/Qhull.h"
#include "libqhullcpp/QhullFacetList.h"
#include "libqhullcpp/QhullError.h"

#include <globalsearch/constants.h>

using namespace orgQhull;

bool distAboveHull(std::vector<double> input_data, int input_num, int input_dim, std::vector<double>& above_hull)
{
  // In this function, we calculate and return the distance above hull for input data.
  //
  // The input data vector, "input_data", is corresponding to data for "input_num" points;
  //   each point's data include the "composition" of "r" elements plus the
  //   "energy" such that each data point has a length of "input_dim = r + 1" and
  //   the length of the vector "input_data" is "input_num * (r+1) = input_num * input_dim".
  // So, the output "above_hull" vector has a length of "input_num".
  //
  // Here we assume that the proper reference points (elemental entries) are included in data.
  //
  // The overall workflow is as follows:
  //   1) Normalize the compositions and energies; meanwhile find elemental references in the data
  //   2) Make sure we have elemental references, and use them to convert input energies to formation energy,
  //   3) Reduce the input data entries by removing the first composition coordinate,
  //   4) Compute the convex hull,
  //   5) Calculate and return the distance above hull for all data points.
  //
  // While processing the input data, we will effectively "overwrite" input data; but this is
  //   done only here for internal use, and we don't want to return any of these changes!

  //===== Process input data
  //minimum elemental enthalpy per atom values
  std::vector<double> min_ele(input_dim - 1, PINF);
  //normalize the entries (just in case); and find elemental references
  for(int i = 0; i < input_num; i++) {
    double tot_com = 0.0;
    for(int j = 0; j < input_dim - 1; j++)
      tot_com += input_data[i * input_dim + j];
    for(int j = 0; j < input_dim; j++)
      input_data[i * input_dim + j] /= tot_com;
    for(int j = 0; j < input_dim - 1; j++)
      if (fabs(input_data[i * input_dim + j] - 1.0) < ZERO6)
        if(input_data[i * input_dim + input_dim - 1] < min_ele[j])
          min_ele[j] = input_data[i * input_dim + input_dim - 1];
  }

  //a sanity check: did we find all elemental references?
  for(int i = 0; i < input_dim - 1; i++)
    if (min_ele[i] == PINF) {
      qDebug().noquote() << "Error: hull - couldn't find all elemental references!";
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
  //initialize reduced hull input vector
  for(int i = 0; i < hll_npnt; i++)
    for(int j = 0; j < hll_ndim; j++)
      hll_inpt.push_back(input_data[i* (hll_ndim + 1) + (j + 1)]);

  //===== Initialize Qhull and run convex hull algorithm
  Qhull qhull;
  try {
    qhull.runQhull("i", hll_ndim, hll_npnt, hll_inpt.data(), "Qt");
  } catch(QhullError &e) {
    qDebug().noquote() << "Error: hull - Qhull had error output!";
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
  if (hll_ndim != hll_eqns[0].size()) {
    qDebug().noquote() << "Error: hull - issue with dimensions!";
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
      if(dotprod > ZERO6 || fabs(hll_eqns[j].back()) < ZERO6)
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
      if (fabs(dist) < ZERO6)
        dist = 0.0;
      //save the distance
      distances.push_back(dist);
    }
    //find the shortest distance among the distances from lower facets
    std::sort(distances.begin(), distances.end());
    //sanity check (we shouldn't have any negative distances at this point!)
    if (distances.size() == 0 || distances[0] < 0.0) {
      qDebug().noquote() << "Error: hull - failed to calculate distance for point " << i+1;
      return false;
    }
    //save the calculated distance above hull
    above_hull[i] = distances[0];
  }

  return true;
}

#endif // CHULL_H
