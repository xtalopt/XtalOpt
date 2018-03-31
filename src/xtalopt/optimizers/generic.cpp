/**********************************************************************
  GenericOptimizer - Tools for a generic optimizer

  Copyright (C) 2018 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <xtalopt/optimizers/generic.h>
#include <xtalopt/structures/xtal.h>
#include <xtalopt/xtalopt.h>

#include <QString>

using namespace GlobalSearch;

namespace XtalOpt {

GenericOptimizer::GenericOptimizer(OptBase* parent, const QString& filename)
  : XtalOptOptimizer(parent)
{
  // Set allowed data structure keys, if any, e.g.
  // None here!

  // Set allowed filenames, e.g.
  m_templates.append("job.in");

  // Setup for completion values
  m_completionFilename = "job.out";
  //m_completionStrings.append("Job Complete");

  // Set output filenames to try to read data from, e.g.
  m_outputFilenames.append(m_completionFilename);

  // Set the name of the optimizer to be returned by getIDString()
  m_idString = "Generic";

  // Local execution setup:
  m_localRunCommand = "";
  m_stdinFilename = "job.in";
  m_stdoutFilename = "job.out";
  m_stderrFilename = "job.err";

  readSettings(filename);
}

} // end namespace XtalOpt
