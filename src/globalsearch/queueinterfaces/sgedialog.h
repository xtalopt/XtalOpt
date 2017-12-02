/**********************************************************************
  SgeConfigDialog -- Setup for remote SGE queues

  Copyright (C) 2011 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef SGECONFIGDIALOG_H
#define SGECONFIGDIALOG_H

#ifdef ENABLE_SSH

// Doxygen should ignore this file:
/// @cond

#include <QDialog>

namespace Ui {
class SgeConfigDialog;
}

namespace GlobalSearch {
class AbstractDialog;
class OptBase;
class SgeQueueInterface;

class SgeConfigDialog : public QDialog
{
  Q_OBJECT

public:
  explicit SgeConfigDialog(AbstractDialog* parent, OptBase* o,
                           SgeQueueInterface* p);
  virtual ~SgeConfigDialog() override;

public slots:
  void updateGUI();

protected slots:
  void accept() override;
  void reject() override;

protected:
  OptBase* m_opt;
  SgeQueueInterface* m_sge;

private:
  Ui::SgeConfigDialog* ui;
};
}

/// @endcond
#endif // ENABLE_SSH
#endif // SGECONFIGDIALOG_H
