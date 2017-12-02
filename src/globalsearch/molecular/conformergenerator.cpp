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

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>

// RDKit files
#include <ForceField/ForceField.h>
#include <GraphMol/DistGeomHelpers/Embedder.h>
#include <GraphMol/FileParsers/FileParsers.h>
#include <GraphMol/FileParsers/MolSupplier.h>
#include <GraphMol/FileParsers/MolWriters.h>
#include <GraphMol/ForceFieldHelpers/MMFF/MMFF.h>
#include <GraphMol/MolAlign/AlignMolecules.h>
#include <GraphMol/ROMol.h> // This has to go before MMFF.h
#include <GraphMol/Substruct/SubstructMatch.h>

#include "conformergenerator.h"

using std::cerr;
using std::cout;
using std::map;
using std::pair;
using std::string;
using std::stringstream;
using std::vector;

// Some implementations of cpp do not have this typedef
typedef unsigned int uint;

namespace GlobalSearch {

/**
 * Get a vector of confIds
 *
 * @param mol The RDKit ROMol for which to get the confIds.
 *
 * @return The vector of confIds.
 */
static vector<int> getConfIds(const RDKit::ROMol& mol)
{
  vector<int> confIds;
  for (auto it = mol.beginConformers(); it != mol.endConformers(); ++it) {
    confIds.push_back((*it)->getId());
  }
  return confIds;
}

/**
 * Compares conformers so that if any two conformers have an RMSD less than
 * the RMSD threshold, the lower energy conformer is removed. @p mol will
 * be updated with the new set of conformers, and @p energies will be updated
 * with their new ids.
 *
 * @param mol The RDKit molecule whose conformers should be pruned.
 * @param energies A map of the conformer ids to their respective energies.
 * @param rmsdThreshold The minimum RMSD to be kept. If two conformers are
 *                      found to have an RMSD below this threshold, the lower
 *                      energy conformer will be kept and the other discarded.
 */
static void pruneConformers(RDKit::ROMol& mol, map<uint, double>& energies,
                            double rmsdThreshold)
{
  vector<int> confIds = getConfIds(mol);

  // Now sort them by energy
  std::sort(confIds.begin(), confIds.end(), [&energies](uint id1, uint id2) {
    return energies[id1] < energies[id2];
  });

  vector<uint> keepConfs;
  for (const auto& id : confIds) {
    // Always keep the first one
    if (keepConfs.empty()) {
      keepConfs.push_back(id);
      continue;
    }

    bool keepThisConformer = true;
    for (const auto& keepId : keepConfs) {
      double rmsd = RDKit::MolAlign::getBestRMS(mol, mol, id, keepId);

      if (rmsd < rmsdThreshold) {
        keepThisConformer = false;
        break;
      }
    }
    if (keepThisConformer)
      keepConfs.push_back(id);
  }

  RDKit::ROMol newMol = mol;
  newMol.clearConformers();

  map<uint, double> newEnergies;
  bool assignId = true;
  for (const auto& keepId : keepConfs) {
    uint newConfId = newMol.addConformer(
      new RDKit::Conformer(mol.getConformer(keepId)), assignId);
    newEnergies[newConfId] = energies[keepId];
  }

  // Unfortunately, we can't do mol = newMol, so we need to
  // copy the conformers back over...
  // energies should stay the same.
  mol.clearConformers();
  for (auto it = newMol.beginConformers(); it != newMol.endConformers(); ++it) {
    mol.addConformer(new RDKit::Conformer(*(*it)), assignId);
  }
  energies = newEnergies;
}

/**
 * Uses RDKit to generate conformers and write them as SDF files to a
 * specific directory.
 *
 * @param sdfFile An istream containing the original molecule whose conformers
 *                are to be found (in SDF format).
 * @param writeDir The directory to which the conformers and their energies
 *                 will be written. The separator at the end ("/" or "\") needs
 *                 to be included. The write dir must also already exist.
 * @param numConformers The number of attempted conformers to generate. The
 *                      resulting number of conformers may be less because of
 *                      RMSD pruning.
 * @param params The RDKit EmbedParameters object that contains all of the
 *               settings for conformer generation.
 * @param maxOptimizationIters The maximum number of iterations when
 *                             optimizing each conformer. This can be set to
 *                             -1 if no optimization is desired.
 * @param pruneConformersAfterOptimization Whether or not to perform an
 *                                         additional pruning after all the
 *                                         conformers have been optimized.
 *                                         The RMSD threshold to be used is
 *                                         in the @p params parameter.
 *
 * @return The number of conformers generated, or -1 if an error occurred
 */

static long long generateRDKitConformers(
  std::istream& sdfFile, const std::string& writeDir, uint numConformers,
  const RDKit::DGeomHelpers::EmbedParameters& params,
  int maxOptimizationIters = 1000, bool pruneConformersAfterOptimization = true)
{
  bool takeOwnership = false, sanitize = true, removeHs = false;
  RDKit::SDMolSupplier supplier(&sdfFile, takeOwnership, sanitize, removeHs);
  RDKit::ROMol* mol = supplier.next();
  if (!mol) {
    std::cerr << "Error: in " << __FUNCTION__ << ": failed to read the SDF "
              << "file\n";
    return -1;
  }

  vector<int> ids =
    RDKit::DGeomHelpers::EmbedMultipleConfs(*mol, numConformers, params);
  cout << ids.size() << " conformers were generated.\n";

  map<uint, double> energies;

  vector<pair<int, double>> res;
  int numThreads = 1;
  string mmffVariant = "MMFF94";
  double nonBondedThresh = 100.0;
  bool ignoreInterfragInteractions = true;
  RDKit::MMFF::MMFFOptimizeMoleculeConfs(
    *mol, res, numThreads, maxOptimizationIters, mmffVariant, nonBondedThresh,
    ignoreInterfragInteractions);

  // Right now, we are ignoring the first item in each element of res
  // that tells us whether or not it needs more optimization steps.
  // We can use that in the future if we so desire.
  for (size_t i = 0; i < res.size(); ++i)
    energies[i] = res[i].second;

  // If we are to prune the conformers after optimization, let's do so
  if (pruneConformersAfterOptimization) {
    pruneConformers(*mol, energies, params.pruneRmsThresh);

    // mol has been edited, so let's get the new conformer ids
    ids = getConfIds(*mol);

    cout << "After pruning, " << ids.size() << " conformers remain.\n";
  }

  // Sort them with respect to energy
  std::sort(ids.begin(), ids.end(), [&energies](uint id1, uint id2) {
    return energies[id1] < energies[id2];
  });

  // Write out the energies to a file
  std::ofstream energiesFile;
  energiesFile.open(writeDir + "energies.txt");
  if (!energiesFile.is_open()) {
    cerr << "Failed to open conformers/energies.txt\n";
  } else {
    for (const auto& id : ids) {
      stringstream ss;
      ss << "conformer-" << id + 1 << ".sdf";
      string conformerName = ss.str();
      energiesFile << std::left << std::setw(20) << conformerName << std::left
                   << std::setw(15) << std::setprecision(8) << energies[id]
                   << "\n";
    }
    energiesFile.close();
  }

  // Now, let's write the sdf files
  for (const auto& id : ids) {
    stringstream ss;
    ss << writeDir << "conformer-" << id + 1 << ".sdf";
    string filename = ss.str();
    RDKit::SDWriter writer(filename);
    writer.write(*mol, id);
  }

  delete mol;

  return ids.size();
}

long long ConformerGenerator::generateConformers(
  std::istream& sdfIstream, const std::string& outDir, size_t numConformers,
  size_t maxOptimizationIters, double rmsdThreshold,
  bool pruneConformersAfterOptimization)
{
  // This uses Sereina Rinkier's ETKDG approach. This method was
  // recommended by RDKit writer, Greg Landrum.
  RDKit::DGeomHelpers::EmbedParameters params(0,       // maxIterations
                                              1,       // numThreads
                                              -1,      // randomSeed
                                              true,    // clearConfs
                                              false,   // useRandomCoords
                                              2.0,     // boxSizeMult
                                              true,    // randNegEig
                                              1,       // numZeroFail
                                              nullptr, // coordMap
                                              1e-3,    // optimizerForceTol
                                              false, // ignoreSmoothingFailures
                                              true,  // enforceChirality
                                              true,  // useExpTorsionAnglePrefs
                                              true,  // useBasicKnowledge
                                              false, // verbose
                                              5.0,   // basinThresh
                                              -1.0,  // pruneRmsThresh
                                              true   // onlyHeavyAtomsForRMS
                                              );

  params.pruneRmsThresh = rmsdThreshold;

  return generateRDKitConformers(sdfIstream, outDir, numConformers, params,
                                 maxOptimizationIters,
                                 pruneConformersAfterOptimization);
}

} // end namespace GlobalSearch
