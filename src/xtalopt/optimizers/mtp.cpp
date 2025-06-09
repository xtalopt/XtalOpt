/**********************************************************************
  MTPOptimizer - Tools to interface with MTP

  Copyright (C) 2025 by Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <xtalopt/optimizers/mtp.h>
#include <xtalopt/structures/xtal.h>
#include <xtalopt/xtalopt.h>

#include <QDir>
#include <QProcess>
#include <QSemaphore>
#include <QString>

using namespace GlobalSearch;

namespace XtalOpt {

MTPOptimizer::MTPOptimizer(SearchBase* parent, const QString& filename)
  : XtalOptOptimizer(parent)
{
  // Set allowed data structure keys, if any, e.g.
  // None here!

  // Set allowed filenames, e.g.
  m_templates.append("mtp.relax");
  m_templates.append("mtp.pot");
  m_templates.append("mtp.cell");

  // Setup for completion values
  m_completionFilename = "xtal.mot_0";
  m_completionStrings.clear();
  m_completionStrings.append("Energy");

  // Set output filenames to try to read data from, e.g.
  m_outputFilenames.append(m_completionFilename);

  // Set the name of the optimizer to be returned by getIDString()
  m_idString = "MTP";

  // Local execution setup:
  m_localRunCommand = "mlp relax mtp.relax --cfg-filename=mtp.cell --save-relaxed=xtal.mot";
  m_stdinFilename = "mtp.cell";
  m_stdoutFilename = "mtp.out";
  m_stderrFilename = "";

  readSettings(filename);
}

} // end namespace XtalOpt
