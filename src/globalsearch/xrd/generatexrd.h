/**********************************************************************
  GenerateXrd - Use ObjCryst++ to generate a simulated x-ray diffraction
                pattern

  Copyright (C) 2018 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef GLOBALSEARCH_GENERATEXRD_H
#define GLOBALSEARCH_GENERATEXRD_H

#include <utility>
#include <vector>

// Declarations
class QByteArray;
class QStringList;

namespace GlobalSearch {

// More declarations
class Structure;

typedef std::vector<std::pair<double, double>> XrdData;

class GenerateXrd
{
public:
  /**
   * The primary function that is to be called in this class. This function
   * takes a Structure object, converts it to a cif file, generates Xrd data
   * from it, and stores it in @p results.
   *
   * @param s The Structure whose XRD is to be obtained.
   * @param results The resulting data (it is a vector of pairs of doubles).
   *                The first element in the pair is the 2theta value in
   *                degrees, and the second element in the pair is the
   *                intensity.
   * @param wavelength The wavelength of the x-ray in Angstroms.
   * @param peakwidth The broadening of the peak at the base (in degrees).
   * @param numpoints The number of 2theta points.
   * @param max2theta The max 2theta value in degrees.
   *
   * @return True on success and false on failure.
   */
  static bool generateXrdPattern(const Structure& s, XrdData& results,
                                 double wavelength = 1.5056,
                                 double peakwidth = 0.52958,
                                 size_t numpoints = 1000,
                                 double max2theta = 162.0);

  /**
   * This function executes the genXrdPattern executable with the given
   * arguments and input, and it puts the output in @p output.
   *
   * @param args The list of arguments in addition to the executable.
   * @param input The stdin input for the program.
   * @param output The output from the program.
   *
   * @return True if the program ran successfully. False if it failed.
   */
  static bool executeGenXrdPattern(const QStringList& args,
                                   const QByteArray& input, QByteArray& output);
};

} // end namespace GlobalSearch

#endif // GLOBALSEARCH_GENERATEXRD_H
