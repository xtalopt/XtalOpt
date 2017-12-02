/**********************************************************************
  ExampleSearch -- A tool for analysing a matrix-substrate docking problem

  Copyright (C) 2012 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <examplesearch/ui/tab_edit.h>

#include <examplesearch/examplesearch.h>
#include <examplesearch/optimizers/gamess.h>
#include <examplesearch/optimizers/mopac.h>
#include <examplesearch/ui/dialog.h>

#include <globalsearch/macros.h>
#include <globalsearch/queueinterfaces/local.h>

#ifdef ENABLE_SSH
#include <globalsearch/queueinterfaces/loadleveler.h>
#include <globalsearch/queueinterfaces/lsf.h>
#include <globalsearch/queueinterfaces/pbs.h>
#include <globalsearch/queueinterfaces/sge.h>
#include <globalsearch/queueinterfaces/slurm.h>
#endif // ENABLE_SSH

#include <QComboBox>
#include <QFileDialog>
#include <QFont>

#include <QSettings>

using namespace GlobalSearch;
using namespace Avogadro;
using namespace std;

namespace ExampleSearch {

TabEdit::TabEdit(ExampleSearchDialog* parent, ExampleSearch* p)
  : DefaultEditTab(parent, p)
{
  // Fill m_optimizers in order of ExampleSearch::OptTypes
  m_optimizers.clear();
  const unsigned int numOptimizers = 2;
  for (unsigned int i = 0; i < numOptimizers; ++i) {
    switch (i) {
      case ExampleSearch::OT_GAMESS:
        m_optimizers.append(new GAMESSOptimizer(m_opt));
        break;
      case ExampleSearch::OT_MOPAC:
        m_optimizers.append(new MopacOptimizer(m_opt));
        break;
    }
  }

  // Fill m_optimizers in order of ExampleSearch::QueueInterfaces
  m_queueInterfaces.clear();
  const unsigned int numQIs =
#ifdef ENABLE_SSH
    6;
#else  // ENABLE_SSH
    1;
#endif // ENABLE_SSH

  for (unsigned int i = 0; i < numQIs; ++i) {
    switch (i) {
      case ExampleSearch::QI_LOCAL:
        m_queueInterfaces.append(new LocalQueueInterface(m_opt));
        break;
#ifdef ENABLE_SSH
      case ExampleSearch::QI_PBS:
        m_queueInterfaces.append(new PbsQueueInterface(m_opt));
        break;
      case ExampleSearch::QI_SGE:
        m_queueInterfaces.append(new SgeQueueInterface(m_opt));
        break;
      case ExampleSearch::QI_LSF:
        m_queueInterfaces.append(new LsfQueueInterface(m_opt));
        break;
      case ExampleSearch::QI_SLURM:
        m_queueInterfaces.append(new SlurmQueueInterface(m_opt));
        break;
      case ExampleSearch::QI_LOADLEVELER:
        m_queueInterfaces.append(new LoadLevelerQueueInterface(m_opt));
        break;
#endif // ENABLE_SSH
    }
  }

  DefaultEditTab::initialize();

  populateTemplates();
}

TabEdit::~TabEdit()
{
}

void TabEdit::writeSettings(const QString& filename)
{
  SETTINGS(filename);

  settings->beginGroup("examplesearch/edit");
  const int version = 1;
  settings->setValue("version", version);

  settings->setValue("description", m_opt->description);
  settings->setValue("localpath", m_opt->filePath);
  settings->setValue("remote/host", m_opt->host);
  settings->setValue("remote/port", m_opt->port);
  settings->setValue("remote/username", m_opt->username);
  settings->setValue("remote/rempath", m_opt->rempath);

  settings->setValue("optimizer", m_opt->optimizer()->getIDString().toLower());
  settings->setValue("queueInterface",
                     m_opt->queueInterface()->getIDString().toLower());

  settings->endGroup();
  m_opt->optimizer()->writeSettings(filename);

  DESTROY_SETTINGS(filename);
}

void TabEdit::readSettings(const QString& filename)
{
  SETTINGS(filename);
  settings->beginGroup("examplesearch/edit");
  int loadedVersion = settings->value("version", 0).toInt();

  m_opt->port = settings->value("remote/port", 22).toInt();

  // Temporary variables to test settings. This prevents empty
  // scheme values from overwriting defaults.
  QString tmpstr;

  tmpstr = settings->value("description", "").toString();
  if (!tmpstr.isEmpty()) {
    m_opt->description = tmpstr;
  }

  tmpstr = settings->value("remote/rempath", "").toString();
  if (!tmpstr.isEmpty()) {
    m_opt->rempath = tmpstr;
  }

  tmpstr = settings->value("localpath", "").toString();
  if (!tmpstr.isEmpty()) {
    m_opt->filePath = tmpstr;
  }

  tmpstr = settings->value("remote/host", "").toString();
  if (!tmpstr.isEmpty()) {
    m_opt->host = tmpstr;
  }

  tmpstr = settings->value("remote/username", "").toString();
  if (!tmpstr.isEmpty()) {
    m_opt->username = tmpstr;
  }

  QString optimizer =
    settings->value("optimizer", "gamess").toString().toLower();
  for (QList<Optimizer *>::const_iterator it = m_optimizers.constBegin(),
                                          it_end = m_optimizers.constEnd();
       it != it_end; ++it) {
    if ((*it)->getIDString().toLower().compare(optimizer) == 0) {
      emit optimizerChanged(*it);
      break;
    }
  }

  QString queueInterface =
    settings->value("queueInterface", "local").toString().toLower();
  for (QList<QueueInterface *>::const_iterator
         it = m_queueInterfaces.constBegin(),
         it_end = m_queueInterfaces.constEnd();
       it != it_end; ++it) {
    if ((*it)->getIDString().toLower().compare(queueInterface) == 0) {
      emit queueInterfaceChanged(*it);
      break;
    }
  }

  settings->endGroup();

  // Update config data
  switch (loadedVersion) {
    case 0:
    case 1:
    default:
      break;
  }

  m_opt->optimizer()->readSettings(filename);
  m_opt->queueInterface()->readSettings(filename);

  updateGUI();
}
}
