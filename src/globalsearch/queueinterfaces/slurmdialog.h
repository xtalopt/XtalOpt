/**********************************************************************
  SlurmConfigDialog -- Setup for remote SLURM queues

  Copyright (C) 2011 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef SLURMCONFIGDIALOG_H
#define SLURMCONFIGDIALOG_H

#ifdef ENABLE_SSH

// Doxygen should ignore this file:
/// @cond

#include <QDialog>

namespace Ui {
class SlurmConfigDialog;
}

namespace GlobalSearch {
class AbstractDialog;
class OptBase;
class SlurmQueueInterface;

class SlurmConfigDialog : public QDialog
{
  Q_OBJECT

public:
  explicit SlurmConfigDialog(AbstractDialog* parent, OptBase* o,
                             SlurmQueueInterface* p);
  virtual ~SlurmConfigDialog() override;

public slots:
  void updateGUI();

protected slots:
  void accept() override;
  void reject() override;

protected:
  OptBase* m_opt;
  SlurmQueueInterface* m_slurm;

private:
  Ui::SlurmConfigDialog* ui;
};
}

/// @endcond
#endif // ENABLE_SSH
#endif // SLURMCONFIGDIALOG_H
