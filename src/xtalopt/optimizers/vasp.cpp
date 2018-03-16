/**********************************************************************
  VASPOptimizer - Tools to interface with VASP

  Copyright (C) 2009-2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <xtalopt/optimizers/vasp.h>
#include <xtalopt/structures/xtal.h>

#include <globalsearch/macros.h>
#include <globalsearch/queueinterface.h>
#include <globalsearch/sshmanager.h>

#include <QDebug>
#include <QDir>
#include <QSettings>
#include <QString>

using namespace GlobalSearch;

namespace XtalOpt {

VASPOptimizer::VASPOptimizer(OptBase* parent, const QString& filename)
  : XtalOptOptimizer(parent)
{
  // Set allowed data structure keys, if any
  // "POTCAR info" is of type
  // QList<QHash<QString, QString> >
  // e.g. a list of hashes containing
  // [atomic symbol : pseudopotential file] pairs
  m_data.insert("POTCAR info", QVariant());
  m_data.insert("Composition", QVariant());

  // Set allowed filenames, e.g.
  m_templates.append("INCAR");
  m_templates.append("POTCAR");
  m_templates.append("KPOINTS");

  // Setup for completion values
  m_completionFilename = "OUTCAR";
  m_completionStrings.clear();
  m_completionStrings.append(
    "General timing and accounting informations for this job:");

  // Set output filenames to try to read data from, e.g.
  m_outputFilenames.append("CONTCAR");
  m_outputFilenames.append("POSCAR");

  // Set the name of the optimizer to be returned by getIDString()
  m_idString = "VASP";

  // Local execution setup:
  m_localRunCommand = "vasp";
  m_stdinFilename = "";
  m_stdoutFilename = "";
  m_stderrFilename = "";

  readSettings(filename);
}

void VASPOptimizer::readSettings(const QString& filename)
{
  // Don't consider default setting,, only schemes and states.
  if (filename.isEmpty())
    return;

  readDataFromSettings(filename);
}

void VASPOptimizer::writeDataToSettings(const QString& filename)
{
  // We only want to save POTCAR info and Composition to the resume
  // file, not the main config file, so only dump the data here if
  // we are given a filename and it contains the string
  // "xtalopt.state"
  if (filename.isEmpty() || !filename.contains("xtalopt.state")) {
    return;
  }
  SETTINGS(filename);

  // Figure out what opt index this is.
  int optInd = m_opt->optimizerIndex(this);
  if (optInd < 0)
    return;

  QStringList ids = getDataIdentifiers();
  for (int i = 0; i < ids.size(); i++) {
    settings->setValue("xtalopt/optimizer/" + getIDString() + "/" +
                       QString::number(optInd) + "/data/" + ids.at(i),
                       m_data.value(ids.at(i)));
  }
}

QHash<QString, QString> VASPOptimizer::getInterpretedTemplates(
  Structure* structure)
{
  QHash<QString, QString> hash = Optimizer::getInterpretedTemplates(structure);
  hash.insert("POSCAR", m_opt->interpretTemplate("%POSCAR%", structure));
  return hash;
}

} // end namespace XtalOpt
