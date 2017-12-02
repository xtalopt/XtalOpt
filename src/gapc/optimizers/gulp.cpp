/**********************************************************************
  GULPOptimizer - Tools to interface with GULP

  Copyright (C) 2009-2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <gapc/optimizers/gulp.h>

using namespace GlobalSearch;

namespace GAPC {

GULPOptimizer::GULPOptimizer(OptBase* parent, const QString& filename)
  : Optimizer(parent)
{
  // Set allowed data structure keys, if any, e.g.
  // None here!

  // Set allowed filenames, e.g.
  m_templates.insert("job.gin", QStringList(""));

  // Setup for completion values
  m_completionFilename = "job.got";
  m_completionStrings.append("**** Optimisation achieved ****");

  // Set output filenames to try to read data from, e.g.
  m_outputFilenames.append(m_completionFilename);

  // Set the name of the optimizer to be returned by getIDString()
  m_idString = "GULP";

  // Local execution setup:
  m_localRunCommand = "gulp";
  m_stdinFilename = "job.gin";
  m_stdoutFilename = "job.got";
  m_stderrFilename = "job.ger";

  readSettings(filename);
}

} // end namespace GAPC
