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

#ifndef COMMON_CHULL_H
#define COMMON_CHULL_H

#include <vector>

namespace Common {

// Find distances above a convex hull. Return false if no hull can be made.
bool distAboveHull(const std::vector<double>& input_data_ref, int input_num, int input_dim,
                   std::vector<double>& above_hull);

// Find the planes of a convex hull. Return false if the points do not make one.
bool convexHullPlanes(const std::vector<double>& points, int numPoints, int dim,
                      std::vector<double>& normals, std::vector<double>& offsets);

// Find the volume of a convex hull. Return false if the points do not make one.
bool convexHullVolume(const std::vector<double>& points, int numPoints, int dim,
                      double& volume);

} // namespace Common

#endif // COMMON_CHULL_H
