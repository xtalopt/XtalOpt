/**********************************************************************
  generatexrd - Generate simulated Xrd pattern

  Copyright (C) 2018 by Patrick Avery
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <generatexrd/generatexrd.h>

#include <common/constants.h>
#include <common/output.h>
#include <common/numericutils.h>
#include <atoms/basis/unitcell.h>
#include <common/matrix.h>
#include <common/vector.h>
#include <atoms/geometry.h>

#include <xraylib.h>


#include <algorithm>
#include <cmath>
#include <complex>
#include <limits>
#include <vector>

namespace GenerateXrd {

namespace {

struct Reflection
{
  double twoTheta;
  double intensity;
};

inline double pi()
{
  return std::acos(-1.0);
}

inline double degreesToRadians(double degrees)
{
  return degrees * pi() / 180.0;
}

inline double radiansToDegrees(double radians)
{
  return radians * 180.0 / pi();
}

// Return an estimated scattering factor when no table value is available.
double approximateAtomicScatteringFactor(unsigned short atomicNumber, double sinThetaOverLambda)
{
  const double z = static_cast<double>(atomicNumber);
  const double s2 = sinThetaOverLambda * sinThetaOverLambda;

  return z * std::exp(-10.0 * s2);
}

double atomicScatteringFactor(unsigned short atomicNumber, double wavelength, double twoTheta)
{
  const double energy = KEV2ANGST / wavelength;
  const double q = MomentTransf(energy, degreesToRadians(twoTheta), nullptr);
  const double f0 = FF_Rayl(static_cast<int>(atomicNumber), q, nullptr);
  if (f0 > 0.0)
    return f0;

  const double theta = degreesToRadians(twoTheta / 2.0);

  return approximateAtomicScatteringFactor(atomicNumber, std::sin(theta) / wavelength);
}

// Return the Lorentz-polarization factor.
double lorentzPolarizationFactor(double twoTheta)
{
  const double theta = degreesToRadians(twoTheta / 2.0);
  const double sinTheta = std::sin(theta);
  const double cosTheta = std::cos(theta);
  const double cosTwoTheta = std::cos(2.0 * theta);

  const double denom = sinTheta * sinTheta * cosTheta;
  if (std::abs(denom) <= ZERO08)
    return 1.0;

  return (1.0 + cosTwoTheta * cosTwoTheta) / denom;
}

std::vector<Reflection> calculateReflections(const Atoms::Geometry& structure,
                                             double wavelength, double max2theta)
{
  std::vector<Reflection> reflections;

  const Atoms::UnitCell& cell = structure.unitCell();
  if (!cell.is3D() || structure.numAtoms() == 0)
    return reflections;

  const double thetaMax = degreesToRadians(max2theta / 2.0);
  const double sinThetaMax = std::sin(thetaMax);
  if (sinThetaMax <= ZERO08)
    return reflections;

  const double minDSpacing = wavelength / (2.0 * sinThetaMax);
  const double maxReciprocalNorm = 1.0 / minDSpacing;

  // Reciprocal cell includes 2*pi factor which here should be divided by.
  // Also, it returns a zero matrix for a singular cell; hence no reflections at all!
  const Common::Matrix3 reciprocal = cell.reciprocalCell() / (2.0 * PI);
  const Common::Vector3 aStar = reciprocal.row(0);
  const Common::Vector3 bStar = reciprocal.row(1);
  const Common::Vector3 cStar = reciprocal.row(2);

  const int maxH = std::max(1, static_cast<int>(std::ceil(maxReciprocalNorm * cell.aVector().norm())));
  const int maxK = std::max(1, static_cast<int>(std::ceil(maxReciprocalNorm * cell.bVector().norm())));
  const int maxL = std::max(1, static_cast<int>(std::ceil(maxReciprocalNorm * cell.cVector().norm())));

  std::vector<Common::Vector3> fractionalCoords;
  fractionalCoords.reserve(structure.numAtoms());
  for (const auto& atom : structure.atoms())
    fractionalCoords.push_back(cell.toFractional(atom.pos()));

  for (int h = -maxH; h <= maxH; ++h) {
    for (int k = -maxK; k <= maxK; ++k) {
      for (int l = -maxL; l <= maxL; ++l) {
        if (h == 0 && k == 0 && l == 0)
          continue;

        const Common::Vector3 reciprocal = static_cast<double>(h) * aStar +
                                   static_cast<double>(k) * bStar + static_cast<double>(l) * cStar;
        const double reciprocalNorm = reciprocal.norm();
        if (reciprocalNorm <= ZERO08 || reciprocalNorm > maxReciprocalNorm + ZERO08)
          continue;

        const double sinTheta = 0.5 * wavelength * reciprocalNorm;
        if (sinTheta <= ZERO08 || sinTheta >= 1.0)
          continue;

        const double theta = std::asin(sinTheta);
        const double twoTheta = radiansToDegrees(2.0 * theta);
        if (twoTheta > max2theta + ZERO08)
          continue;

        std::complex<double> structureFactor(0.0, 0.0);

        for (size_t i = 0; i < structure.numAtoms(); ++i) {
          const auto& atom = structure.atom(i);
          const Common::Vector3& frac = fractionalCoords[i];
          const double phase = 2.0 * pi() * (static_cast<double>(h) * frac.x() +
                                static_cast<double>(k) * frac.y() +
                                static_cast<double>(l) * frac.z());
          const double f = atomicScatteringFactor(atom.atomicNumber(), wavelength, twoTheta);
          structureFactor += std::polar(f, phase);
        }

        const double intensity = std::norm(structureFactor) * lorentzPolarizationFactor(twoTheta);
        if (intensity <= ZERO08)
          continue;

        reflections.push_back({twoTheta, intensity});
      }
    }
  }

  return reflections;
}

std::vector<Reflection> mergeEquivalentReflections(std::vector<Reflection> reflections)
{
  if (reflections.empty())
    return reflections;

  std::sort(reflections.begin(), reflections.end(),
            [](const Reflection& lhs, const Reflection& rhs) {
              return lhs.twoTheta < rhs.twoTheta;
            });

  std::vector<Reflection> merged;
  merged.reserve(reflections.size());

  Reflection current = reflections.front();

  for (size_t i = 1; i < reflections.size(); ++i) {
    const Reflection& next = reflections[i];
    if (Common::eq(next.twoTheta, current.twoTheta, XRD_MERGE_TOL)) {
      const double totalIntensity = current.intensity + next.intensity;
      if (totalIntensity > ZERO08) {
        current.twoTheta = (current.twoTheta * current.intensity + next.twoTheta * next.intensity) /
          totalIntensity;
      }
      current.intensity = totalIntensity;
    } else {
      merged.push_back(current);
      current = next;
    }
  }

  merged.push_back(current);

  return merged;
}

void broadenReflections(const std::vector<Reflection>& reflections, XrdData& results,
                        double peakwidth, size_t numpoints, double max2theta)
{
  results.clear();
  results.reserve(numpoints);

  if (numpoints == 0)
    return;

  const double step = max2theta / static_cast<double>(numpoints);
  std::vector<double> intensities(numpoints, 0.0);

  if (peakwidth <= ZERO08) {
    for (const auto& reflection : reflections) {
      const size_t index = static_cast<size_t>(std::min<double>(numpoints - 1,
                         std::max<double>(0.0, std::round(reflection.twoTheta / step))));
      intensities[index] += reflection.intensity;
    }
  } else {
    const double sigma = std::max(step, peakwidth / 6.0);
    const double radius = 4.0 * sigma;

    for (const auto& reflection : reflections) {
      const int begin =
        std::max(0, static_cast<int>(std::floor((reflection.twoTheta - radius) / step)));
      const int end = std::min(static_cast<int>(numpoints) - 1,
                 static_cast<int>(std::ceil((reflection.twoTheta + radius) / step)));

      for (int i = begin; i <= end; ++i) {
        const double twoTheta = step * static_cast<double>(i);
        const double delta = (twoTheta - reflection.twoTheta) / sigma;
        intensities[static_cast<size_t>(i)] +=
          reflection.intensity * std::exp(-0.5 * delta * delta);
      }
    }
  }

  const double maxIntensity = *std::max_element(intensities.begin(), intensities.end());
  const double scale = (maxIntensity > ZERO08) ? 100.0 / maxIntensity : 1.0;

  for (size_t i = 0; i < numpoints; ++i) {
    results.push_back(std::make_pair(step * static_cast<double>(i), intensities[i] * scale));
  }
}

} // namespace

bool generatePattern(const Atoms::Geometry& structure, XrdData& results, double wavelength,
                     double peakwidth, size_t numpoints, double max2theta)
{
  const QString functionName("generatePattern");
  if (!structure.is3D()) {
    Common::error(QString("%1: structure does not have a 3D unit cell")
                 .arg(functionName));
    return false;
  }

  if (structure.numAtoms() == 0) {
    Common::error(QString("%1: structure does not contain atoms")
                 .arg(functionName));
    return false;
  }

  if (wavelength <= ZERO08 || numpoints == 0 ||
      max2theta <= ZERO08 || max2theta > 180.0) {
    Common::error(QString("%1: invalid XRD options").arg(functionName));
    return false;
  }

  std::vector<Reflection> reflections = calculateReflections(structure, wavelength, max2theta);
  reflections = mergeEquivalentReflections(std::move(reflections));

  if (reflections.empty()) {
    Common::error(QString("%1: no reflections were generated for structure")
                 .arg(functionName));
    return false;
  }

  broadenReflections(reflections, results, peakwidth, numpoints, max2theta);

  return !results.empty();
}

} // namespace GenerateXrd
