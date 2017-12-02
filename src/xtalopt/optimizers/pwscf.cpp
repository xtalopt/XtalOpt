/**********************************************************************
  PWscfOptimizer - Tools to interface with PWscf

  Copyright (C) 2009-2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <xtalopt/optimizers/pwscf.h>

#include <QDebug>
#include <QDir>
#include <QString>

using namespace GlobalSearch;

namespace XtalOpt {

PWscfOptimizer::PWscfOptimizer(OptBase* parent, const QString& filename)
  : XtalOptOptimizer(parent)
{
  // Set allowed data structure keys, if any, e.g.
  // None here!

  // Set allowed filenames, e.g.
  m_templates.append("xtal.in");

  // Setup for completion values
  m_completionFilename = "xtal.out";
  m_completionStrings.clear();
  m_completionStrings.append("Final");

  // Set output filenames to try to read data from, e.g.
  m_outputFilenames.append(m_completionFilename);

  // Set the name of the optimizer to be returned by getIDString()
  m_idString = "PWscf";

  // Local execution setup:
  m_localRunCommand = "pw.x";
  m_stdinFilename = "xtal.in";
  m_stdoutFilename = "xtal.out";
  m_stderrFilename = "xtal.err";

  readSettings(filename);
}

} // end namespace XtalOpt
