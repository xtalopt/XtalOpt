/**********************************************************************
  ADFOptimizer - Tools to interface with ADF remotely

  Copyright (C) 2009-2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <gapc/optimizers/adf.h>

#include <globalsearch/structure.h>

#include <openbabel/mol.h>
#include <openbabel/obconversion.h>

#include <QDebug>
#include <QDir>
#include <QString>

using namespace GlobalSearch;

namespace GAPC {

ADFOptimizer::ADFOptimizer(GlobalSearch::OptBase* parent,
                           const QString& filename)
  : Optimizer(parent)
{
  // Set allowed data structure keys, if any, e.g.
  // None here!

  // Set allowed filenames, e.g.
  m_templates.insert("job.adfin", QStringList(""));

  // Setup for completion values
  m_completionFilename = "job.adfout";
  m_completionStrings.clear();
  // m_completionStrings is not used -- see checkForSuccessfulOutput
  // reimplementation for details.

  // Set output filenames to try to read data from
  m_outputFilenames.append(m_completionFilename);

  // Set the name of the optimizer to be returned by getIDString()
  m_idString = "ADF";

  // Local execution setup:
  m_localRunCommand = "adf";
  m_stdinFilename = "job.adfin";
  m_stdoutFilename = "job.adfout";
  m_stderrFilename = "job.adferr";

  readSettings(filename);
}

bool ADFOptimizer::checkForSuccessfulOutput(Structure* s, bool* success)
{
  int ec;
  *success = false;

  // Check that the output contains:
  // "GEOMETRY CONVERGED"
  // but not
  // "ERROR: STOP GEOMETRY ITERATIONS"
  // ADF will print both when SCF fails to converge.
  if (!m_opt->queueInterface()->grepFile(s, "GEOMETRY CONVERGED",
                                         m_completionFilename, 0, &ec) ||
      ec == 2) {
    return false;
  }
  // ec 0: Match found
  if (ec == 0) {
    if (!m_opt->queueInterface()->grepFile(s, "ERROR: STOP GEOMETRY ITERATIONS",
                                           m_completionFilename, 0, &ec) ||
        ec == 2) {
      return false;
    }
    // ec 1: No match, successful execution
    if (ec == 1) {
      *success = true;
      return true;
    }
    // Check ran correctly, but no successful output found. Return
    // false, success is still false from initializaion.
    return true;
  }
}

} // end namespace GAPC
