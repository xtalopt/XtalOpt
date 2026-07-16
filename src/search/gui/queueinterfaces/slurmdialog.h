/**********************************************************************
  SlurmConfigDialog - Setup for SLURM queues

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

// Doxygen should ignore this file:
/// @cond

#include <QDialog>

namespace Ui {
/**
 * Settings dialog for the SLURM queue.
 */
class SlurmConfigDialog;
}

namespace Search {
class SearchBase;
class SlurmQueueInterface;

class SlurmConfigDialog : public QDialog
{
  Q_OBJECT

public:
  explicit SlurmConfigDialog(QWidget* parent, SearchBase* o, SlurmQueueInterface* p);
  virtual ~SlurmConfigDialog() override;

public slots:
  void updateGUI();

protected slots:
  void accept() override;
  void reject() override;

protected:
  SearchBase* m_search;
  SlurmQueueInterface* m_slurm;

private:
  Ui::SlurmConfigDialog* ui;
};
}

/// @endcond
#endif // SLURMCONFIGDIALOG_H
