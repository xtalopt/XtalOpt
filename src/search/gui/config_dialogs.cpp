/**********************************************************************
  config_dialogs - Handler for creating optimizers and queue interfaces

  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <search/gui/config_dialogs.h>

#include <search/optimizer.h>
#include <search/gui/optimizerdialog.h>
#include <search/queueinterface.h>
#include <search/queueinterfaces/directrun.h>
#include <search/queueinterfaces/loadleveler.h>
#include <search/queueinterfaces/lsf.h>
#include <search/queueinterfaces/pbs.h>
#include <search/queueinterfaces/sge.h>
#include <search/queueinterfaces/slurm.h>
#include <search/gui/queueinterfaces/loadlevelerdialog.h>
#include <search/gui/queueinterfaces/lsfdialog.h>
#include <search/gui/queueinterfaces/pbsdialog.h>
#include <search/gui/queueinterfaces/sgedialog.h>
#include <search/gui/queueinterfaces/slurmdialog.h>

namespace Search {

bool hasOptimizerEditor(const Optimizer* optimizer, const QueueInterface* queueInterface)
{
  return optimizer != nullptr && qobject_cast<const DirectRunInterface*>(queueInterface) != nullptr;
}

QDialog* createOptimizerEditor(QWidget* parent, SearchBase* search, Optimizer* optimizer,
                               QueueInterface* queueInterface)
{
  if (!hasOptimizerEditor(optimizer, queueInterface))
    return nullptr;

  auto* dialog = new OptimizerConfigDialog(parent, search, optimizer);
  dialog->updateGUI();
  return dialog;
}

bool hasQueueInterfaceEditor(const QueueInterface* queueInterface)
{
  return qobject_cast<const PbsQueueInterface*>(queueInterface) ||
         qobject_cast<const SgeQueueInterface*>(queueInterface) ||
         qobject_cast<const SlurmQueueInterface*>(queueInterface) ||
         qobject_cast<const LsfQueueInterface*>(queueInterface) ||
         qobject_cast<const LoadLevelerQueueInterface*>(queueInterface);
}

QDialog* createQueueInterfaceEditor(QWidget* parent, SearchBase* search,
                                    QueueInterface* queueInterface)
{
  if (auto* pbs = qobject_cast<PbsQueueInterface*>(queueInterface)) {
    auto* dialog = new PbsConfigDialog(parent, search, pbs);
    dialog->updateGUI();
    return dialog;
  }
  if (auto* sge = qobject_cast<SgeQueueInterface*>(queueInterface)) {
    auto* dialog = new SgeConfigDialog(parent, search, sge);
    dialog->updateGUI();
    return dialog;
  }
  if (auto* slurm = qobject_cast<SlurmQueueInterface*>(queueInterface)) {
    auto* dialog = new SlurmConfigDialog(parent, search, slurm);
    dialog->updateGUI();
    return dialog;
  }
  if (auto* lsf = qobject_cast<LsfQueueInterface*>(queueInterface)) {
    auto* dialog = new LsfConfigDialog(parent, search, lsf);
    dialog->updateGUI();
    return dialog;
  }
  if (auto* loadLeveler = qobject_cast<LoadLevelerQueueInterface*>(queueInterface)) {
    auto* dialog = new LoadLevelerConfigDialog(parent, search, loadLeveler);
    dialog->updateGUI();
    return dialog;
  }

  return nullptr;
}

} // namespace Search
