/**********************************************************************
  GAMESSOptimizer - Tools to interface with GAMESS remotely

  Copyright (C) 2012 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <examplesearch/optimizers/gamess.h>

#include <globalsearch/structure.h>

using namespace GlobalSearch;

namespace ExampleSearch {

GAMESSOptimizer::GAMESSOptimizer(OptBase* parent, const QString& filename)
  : Optimizer(parent)
{
  // Set allowed data structure keys, if any, e.g.
  // None here!

  // Set allowed filenames, e.g.
  m_templates.insert("job.inp", QStringList(""));

  // Setup for completion values
  m_completionFilename = "job.gamout";
  m_completionStrings.clear();
  m_completionStrings.append("***** EQUILIBRIUM GEOMETRY LOCATED *****");

  // Set output filenames to try to read data from
  m_outputFilenames.append(m_completionFilename);

  // Set the name of the optimizer to be returned by getIDString()
  m_idString = "GAMESS";

  // Local execution setup:
  m_localRunCommand = "gms job";
  m_stdinFilename = "";
  m_stdoutFilename = "";
  m_stderrFilename = "";

  readSettings(filename);
}

} // end namespace ExampleSearch
