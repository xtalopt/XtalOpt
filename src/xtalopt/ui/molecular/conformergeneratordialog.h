/**********************************************************************
  ConformerGeneratorDialog -- Generate conformers with RDKit

  Copyright (C) 2017 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef CONFORMER_GENERATOR_DIALOG_H
#define CONFORMER_GENERATOR_DIALOG_H

#ifdef ENABLE_MOLECULAR

// Doxygen should ignore this file:
/// @cond

#include <QDialog>

namespace Ui {
class ConformerGeneratorDialog;
}

namespace GlobalSearch {
class OptBase;

class ConformerGeneratorDialog : public QDialog
{
  Q_OBJECT

public:
  explicit ConformerGeneratorDialog(QDialog* parent, OptBase* o);
  virtual ~ConformerGeneratorDialog() override;

public slots:
  void updateGUI();

protected slots:
  void accept() override;
  void reject() override;

  void browseInitialMolFile();
  void browseConformerOutDir();

protected:
  OptBase* m_opt;

private:
  Ui::ConformerGeneratorDialog* ui;
};
}

/// @endcond
#endif // ENABLE_MOLECULAR
#endif // CONFORMER_GENERATOR_DIALOG_H
