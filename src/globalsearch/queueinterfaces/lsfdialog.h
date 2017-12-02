/**********************************************************************
  LsfConfigDialog -- Setup for remote LSF queues

  Copyright (C) 2011 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef LSFCONFIGDIALOG_H
#define LSFCONFIGDIALOG_H

#ifdef ENABLE_SSH

// Doxygen should ignore this file:
/// @cond

#include <QDialog>

namespace Ui {
class LsfConfigDialog;
}

namespace GlobalSearch {
class AbstractDialog;
class OptBase;
class LsfQueueInterface;

class LsfConfigDialog : public QDialog
{
  Q_OBJECT

public:
  explicit LsfConfigDialog(AbstractDialog* parent, OptBase* o,
                           LsfQueueInterface* p);
  virtual ~LsfConfigDialog() override;

public slots:
  void updateGUI();

protected slots:
  void accept() override;
  void reject() override;

protected:
  OptBase* m_opt;
  LsfQueueInterface* m_lsf;

private:
  Ui::LsfConfigDialog* ui;
};
}

/// @endcond
#endif // ENABLE_SSH
#endif // LSFCONFIGDIALOG_H
