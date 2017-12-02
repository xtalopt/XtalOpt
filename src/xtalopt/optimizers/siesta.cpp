/**********************************************************************
  SIESTAOptimizer - Tools to interface with SIESTA

  Copyright (C) 2009-2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <xtalopt/optimizers/siesta.h>
#include <xtalopt/structures/xtal.h>
#include <xtalopt/xtalopt.h>

#include <QDebug>
#include <QDir>
#include <QSettings>
#include <QString>

using namespace GlobalSearch;

namespace XtalOpt {

SIESTAOptimizer::SIESTAOptimizer(OptBase* parent, const QString& filename)
  : XtalOptOptimizer(parent)
{
  // Set allowed data structure keys, if any
  m_data.insert("PSF info", QVariant());
  m_data.insert("Composition", QVariant());

  // Set allowed filenames, e.g.
  m_templates.append("xtal.fdf");

  // Setup for completion values
  m_completionFilename = "xtal.out";
  m_completionStrings.append("siesta: Final energy (eV):");

  // Set output filenames to try to read data from, e.g.
  m_outputFilenames.append(m_completionFilename);

  // Set the name of the optimizer to be returned by getIDString()
  m_idString = "SIESTA";

  // Local execution setup:
  m_localRunCommand = "siesta";
  m_stdinFilename = "xtal.fdf";
  m_stdoutFilename = "xtal.out";
  m_stderrFilename = "";

  readSettings(filename);
}

QHash<QString, QString> SIESTAOptimizer::getInterpretedTemplates(
  Structure* structure)
{
  QHash<QString, QString> hash = Optimizer::getInterpretedTemplates(structure);
  return hash;
}
} // end namespace XtalOpt
