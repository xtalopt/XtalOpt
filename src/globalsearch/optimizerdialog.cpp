/**********************************************************************
  OptimizerDialog - Generic optimizer configuration dialog

  Copyright (C) 2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

// Don't document this:
/// @cond

#include <globalsearch/optimizerdialog.h>

#include <globalsearch/optbase.h>
#include <globalsearch/optimizer.h>
#include <globalsearch/ui/abstractdialog.h>

#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpacerItem>
#include <QVBoxLayout>

#include <QObject>

namespace GlobalSearch {

OptimizerConfigDialog::OptimizerConfigDialog(AbstractDialog* parent,
                                             OptBase* opt, Optimizer* o)
  : QDialog(parent), m_opt(opt), m_optimizer(o), m_lineedit(0)
{
  QVBoxLayout* vlayout = new QVBoxLayout(this);

  QLabel* label =
    new QLabel(tr("Local path to %1 executable "
                  "(only needed when using local queue interface):")
                 .arg(m_optimizer->m_idString),
               this);
  vlayout->addWidget(label);

  m_lineedit = new QLineEdit(this);
  vlayout->addWidget(m_lineedit);

  QSpacerItem* spacer =
    new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding);
  vlayout->addItem(spacer);

  QDialogButtonBox* bbox = new QDialogButtonBox(this);
  bbox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  vlayout->addWidget(bbox);

  setLayout(vlayout);

  connect(bbox, SIGNAL(accepted()), this, SLOT(updateState()));
  connect(bbox, SIGNAL(accepted()), this, SLOT(close()));
  connect(bbox, SIGNAL(rejected()), this, SLOT(updateGUI()));
  connect(bbox, SIGNAL(rejected()), this, SLOT(close()));
}

void OptimizerConfigDialog::updateState()
{
  m_optimizer->m_localRunCommand = m_lineedit->text();
}

void OptimizerConfigDialog::updateGUI()
{
  m_lineedit->setText(m_optimizer->m_localRunCommand);
}

} // end namespace GlobalSearch

/// @endcond
