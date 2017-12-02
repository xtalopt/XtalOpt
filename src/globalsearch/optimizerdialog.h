/**********************************************************************
  Optimizer - Generic optimizer interface

  Copyright (C) 2010 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

// Don't document this:
/// @cond
#ifndef OPTIMIZERDIALOG_H
#define OPTIMIZERDIALOG_H

#include <QDialog>

class QLineEdit;

namespace GlobalSearch {
class AbstractDialog;
class OptBase;
class Optimizer;

// Basic input dialog needed for all optimizers
class OptimizerConfigDialog : public QDialog
{
  Q_OBJECT
public:
  OptimizerConfigDialog(AbstractDialog* parent, OptBase* opt, Optimizer* o);

public slots:
  void updateState();
  void updateGUI();

protected:
  OptBase* m_opt;
  Optimizer* m_optimizer;
  QLineEdit* m_lineedit;
};

} // end namespace GlobalSearch

#endif
/// @endcond
