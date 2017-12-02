/**********************************************************************
  TabEdit - Interface to edit optimization templates

  Copyright (C) 2009-2011 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <gapc/ui/tab_edit.h>

#include <gapc/gapc.h>
#include <gapc/optimizers/adf.h>
#include <gapc/optimizers/gulp.h>
#include <gapc/ui/dialog.h>

#include <globalsearch/macros.h>
#include <globalsearch/queueinterfaces/local.h>

#ifdef ENABLE_SSH
#include <globalsearch/queueinterfaces/pbs.h>
#include <globalsearch/queueinterfaces/sge.h>
#endif // ENABLE_SSH

#include <QComboBox>

using namespace GlobalSearch;
using namespace std;

namespace GAPC {
TabEdit::TabEdit(GAPCDialog* parent, OptGAPC* p) : DefaultEditTab(parent, p)
{
  // Fill m_optimizers in order of GAPC::OptTypes
  m_optimizers.clear();
  const unsigned int numOptimizers = 2;
  for (unsigned int i = 0; i < numOptimizers; ++i) {
    switch (i) {
      case OptGAPC::OT_ADF:
        m_optimizers.append(new ADFOptimizer(m_opt));
        break;
      case OptGAPC::OT_GULP:
        m_optimizers.append(new GULPOptimizer(m_opt));
        break;
    }
  }

  // Fill m_optimizers in order of GAPC::QueueInterfaces
  m_queueInterfaces.clear();
  const unsigned int numQIs = 3;
  for (unsigned int i = 0; i < numQIs; ++i) {
    switch (i) {
      case OptGAPC::QI_LOCAL:
        m_queueInterfaces.append(new LocalQueueInterface(m_opt));
        break;
#ifdef ENABLE_SSH
      case OptGAPC::QI_PBS:
        m_queueInterfaces.append(new PbsQueueInterface(m_opt));
        break;
      case OptGAPC::QI_SGE:
        m_queueInterfaces.append(new SgeQueueInterface(m_opt));
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

  settings->beginGroup("gapc/edit");
  const int version = 2;
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
  settings->beginGroup("gapc/edit");
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

  if (loadedVersion >= 2) {
    QString optimizer =
      settings->value("optimizer", "gulp").toString().toLower();
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
  }

  settings->endGroup();

  // Update config data
  switch (loadedVersion) {
    case 0:
    case 1: // Renamed optType to optimizer, added
            // queueInterface. Both now use lowercase strings to
            // identify. Took ownership of variables previously held
            // by tabsys.
      {
#ifdef ENABLE_SSH
        // Extract optimizer ID and subtract 1 -- we removed the
        // openbabel optimizer (was at enum value 0)
        // Also make default value 1 to reflect OB's removal.
        ui_combo_optimizers->setCurrentIndex(
          settings->value("gapc/edit/optType", 1).toInt() - 1);
        // Set QueueInterface based on optimizer
        switch (ui_combo_optimizers->currentIndex()) {
          default:
          case OptGAPC::OT_ADF:
            ui_combo_queueInterfaces->setCurrentIndex(OptGAPC::QI_PBS);
            // Copy over job.pbs
            settings->setValue(
              "gapc/optimizer/ADF/QI/PBS/job.pbs_list",
              settings->value("gapc/optimizer/ADF/job.pbs_list",
                              QStringList("")));
            break;
          case OptGAPC::OT_GULP:
            ui_combo_queueInterfaces->setCurrentIndex(OptGAPC::QI_LOCAL);
            break;
        }
#endif // ENABLE_SSH

        // Formerly tab_sys stuff. Read from default settings object:
        settings->beginGroup("gapc/sys/");
        m_opt->description = settings->value("description", "").toString();
        m_opt->rempath = settings->value("remote/rempath", "").toString();
        m_opt->filePath = settings->value("file/path", "").toString();
        m_opt->host = settings->value("remote/host", "").toString();
        m_opt->port = settings->value("remote/port", 22).toInt();
        m_opt->username = settings->value("remote/username", "").toString();
        m_opt->rempath = settings->value("remote/rempath", "").toString();
        settings->endGroup(); // "gapc/sys"
      }
    case 2:
    default:
      break;
  }

  m_opt->optimizer()->readSettings(filename);
  m_opt->queueInterface()->readSettings(filename);

  updateGUI();
}
}
