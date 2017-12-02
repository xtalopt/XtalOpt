/**********************************************************************
  CASTEPOptimizer - Tools to interface with CASTEP

  Copyright (C) 2009-2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <xtalopt/optimizers/castep.h>

#include <QDebug>
#include <QDir>
#include <QString>

using namespace std;

namespace XtalOpt {

CASTEPOptimizer::CASTEPOptimizer(GlobalSearch::OptBase* parent,
                                 const QString& filename)
  : XtalOptOptimizer(parent)
{
  // Set allowed data structure keys, if any, e.g.
  // None here!

  // Set allowed filenames, e.g.
  m_templates.append("xtal.param");
  m_templates.append("xtal.cell");

  // Setup for completion values
  m_completionFilename = "xtal.castep";
  m_completionStrings.clear();
  m_completionStrings.append("Geometry optimization completed successfully.");

  // Set output filenames to try to read data from, e.g.
  m_outputFilenames.append(m_completionFilename);

  // Set the name of the optimizer to be returned by getIDString()
  m_idString = "CASTEP";

  // Local execution setup:
  m_localRunCommand = "castep xtal";
  m_stdinFilename = "";
  m_stdoutFilename = "";
  m_stderrFilename = "";

  readSettings(filename);
}

} // end namespace XtalOpt
