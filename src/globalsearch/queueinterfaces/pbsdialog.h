/**********************************************************************
  PbsConfigDialog -- Setup for remote PBS queues

  Copyright (C) 2011 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef PBSCONFIGDIALOG_H
#define PBSCONFIGDIALOG_H

#ifdef ENABLE_SSH

// Doxygen should ignore this file:
/// @cond

#include <QDialog>

namespace Ui {
class PbsConfigDialog;
}

namespace GlobalSearch {
class AbstractDialog;
class OptBase;
class PbsQueueInterface;

class PbsConfigDialog : public QDialog
{
  Q_OBJECT

public:
  explicit PbsConfigDialog(AbstractDialog* parent, OptBase* o,
                           PbsQueueInterface* p);
  virtual ~PbsConfigDialog() override;

public slots:
  void updateGUI();

protected slots:
  void accept() override;
  void reject() override;

protected:
  OptBase* m_opt;
  PbsQueueInterface* m_pbs;

private:
  Ui::PbsConfigDialog* ui;
};
}

/// @endcond
#endif // ENABLE_SSH
#endif // PBSCONFIGDIALOG_H
