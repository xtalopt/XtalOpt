/**********************************************************************
  ConformerGenerator - a static class to generate conformers through the use
                       of RDKit

  Copyright (C) 2017 by Patrick S. Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

 ***********************************************************************/

#ifndef GLOBALSEARCH_CONFORMER_GENERATOR_H
#define GLOBALSEARCH_CONFORMER_GENERATOR_H

#include <istream>
#include <string>

namespace GlobalSearch {

class ConformerGenerator
{
public:
  /**
   * Uses RDKit to generate conformers and write them as SDF files to a
   * specific directory. The energies will also be written to an "energies.txt"
   * file like so:
   *
   *    conformer-1.sdf     -5.0759733
   *    conformer-2.sdf     -4.2937643
   *    conformer-3.sdf     -4.2937643
   *    ...
   *
   * @param sdfIstream An istream in containing the original molecule (in SDF
   *                   format) whose conformers are to be found.
   * @param outDir The directory to which the conformers and their energies
   *               will be written. The write dir must already exist. The
   *               energies will be written in "energies.txt" within this
   *               directory, and they will be sorted from lowest to highest
   *               energy and labelled with their respective indices. The
   *               conformers will be written as SDF files as
   *               "conformer-<num>.sdf".
   * @param numConformers The number of attempted conformers to generate. The
   *                      resulting number of conformers may be less because of
   *                      RMSD pruning.
   * @param maxOptimizationIters The maximum number of iterations when
   *                             optimizing each conformer. This can be set to
   *                             -1 if no optimization is desired.
   * @param rmsdThreshold The maximum rmsd threshold that two conformers can
   *                      have before one of them is discarded. RDKit will
   *                      use this threshold to discard identical conformers
   *                      when it first generates them, and if
   *                      @p pruneConformersAfterOptimization is used,
   *                      @p rmsdThreshold will also be used to determine
   *                      whether to discard two optimized conformers.
   * @param pruneConformersAfterOptimization Whether or not to perform an
   *                                         additional pruning after all the
   *                                         conformers have been optimized.
   *                                         The RMSD threshold to be used is
   *                                         the same as the @p rmsdThreshold
   *                                         parameter.
   *
   * @return The number of conformers that were generated (after pruning),
   *         or -1 if an error occurred.
   */
  static long long generateConformers(
    std::istream& sdfIstream, const std::string& outDir,
    size_t numConformers = 1000, size_t maxOptimizationIters = 1000,
    double rmsdThreshold = 0.1, bool pruneConformersAfterOptimization = true);
};
}

#endif
