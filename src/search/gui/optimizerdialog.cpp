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

#include <search/gui/optimizerdialog.h>

#include <search/search.h>
#include <search/optimizer.h>

#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpacerItem>
#include <QVBoxLayout>

#include <QObject>

namespace Search {

OptimizerConfigDialog::OptimizerConfigDialog(QWidget* parent, SearchBase* srch, Optimizer* o)
  : QDialog(parent), m_search(srch), m_optimizer(o), m_lineedit(0)
{
  QVBoxLayout* vlayout = new QVBoxLayout(this);

  QLabel* label =
    new QLabel(tr("Direct run command for %1 "
                 "(used when queueInterface=none):")
                 .arg(m_optimizer->getIDString()),
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

  connect(bbox, &QDialogButtonBox::accepted, this, &OptimizerConfigDialog::updateState);
  connect(bbox, &QDialogButtonBox::accepted, this, &OptimizerConfigDialog::close);
  connect(bbox, &QDialogButtonBox::rejected, this, &OptimizerConfigDialog::updateGUI);
  connect(bbox, &QDialogButtonBox::rejected, this, &OptimizerConfigDialog::close);
}

void OptimizerConfigDialog::updateState()
{
  if (m_search->isSessionInProgress())
    return;
  m_optimizer->setDirectRunCommand(m_lineedit->text());
}

void OptimizerConfigDialog::updateGUI()
{
  m_lineedit->setText(m_optimizer->getDirectRunCommand());
}

} // end namespace Search

/// @endcond
