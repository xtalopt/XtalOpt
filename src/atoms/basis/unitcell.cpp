/**********************************************************************
  Unit Cell - A basic unit cell class.

  Copyright (C) 2017 by Patrick S. Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <atoms/basis/unitcell.h>

namespace Atoms {
void UnitCell::setCellParameters(double a, double b, double c, double alpha,
                                 double beta, double gamma)
{
  const double cosAlpha = std::cos(alpha * DEG2RAD);
  const double cosBeta = std::cos(beta * DEG2RAD);
  const double cosGamma = std::cos(gamma * DEG2RAD);
  const double sinGamma = std::sin(gamma * DEG2RAD);

  u_cellMatrix(0, 0) = a;
  u_cellMatrix(0, 1) = 0.0;
  u_cellMatrix(0, 2) = 0.0;

  u_cellMatrix(1, 0) = b * cosGamma;
  u_cellMatrix(1, 1) = b * sinGamma;
  u_cellMatrix(1, 2) = 0.0;

  u_cellMatrix(2, 0) = c * cosBeta;
  u_cellMatrix(2, 1) = c * (cosAlpha - cosBeta * cosGamma) / sinGamma;
  u_cellMatrix(2, 2) =
    (c / sinGamma) *
    std::sqrt(1.0 - ((cosAlpha * cosAlpha) + (cosBeta * cosBeta) +
                     (cosGamma * cosGamma)) +
              (2.0 * cosAlpha * cosBeta * cosGamma));

  u_periodic[0] = u_periodic[1] = u_periodic[2] = true;
}

Common::Vector3 UnitCell::wrapFractional(const Common::Vector3& frac) const
{
  Common::Vector3 ret = Common::Vector3(std::fmod(frac[0], 1.0), std::fmod(frac[1], 1.0),
                        std::fmod(frac[2], 1.0));
  if (ret[0] < 0.0)
    ++ret[0];
  if (ret[1] < 0.0)
    ++ret[1];
  if (ret[2] < 0.0)
    ++ret[2];
  return ret;
}

Common::Vector3 UnitCell::minimumImageFractional(const Common::Vector3& frac)
{
  double x = frac[0] - rint(frac[0]);
  double y = frac[1] - rint(frac[1]);
  double z = frac[2] - rint(frac[2]);
  return Common::Vector3(x, y, z);
}
}
