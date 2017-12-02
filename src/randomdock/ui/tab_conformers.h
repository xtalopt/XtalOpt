/**********************************************************************
  RandomDock -- A tool for analysing a matrix-substrate docking problem

  Copyright (C) 2009-2011 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef TAB_CONFORMERS_H
#define TAB_CONFORMERS_H

#include <globalsearch/ui/abstracttab.h>

#include "ui_tab_conformers.h"

#include <QMutex>

namespace GlobalSearch {
class Structure;
}

namespace Avogadro {
class Molecule;
}

namespace OpenBabel {
class OBForceField;
}

namespace RandomDock {
class RandomDockDialog;
class RandomDock;

class TabConformers : public GlobalSearch::AbstractTab
{
  Q_OBJECT

public:
  explicit TabConformers(RandomDockDialog* dialog, RandomDock* opt);
  virtual ~TabConformers();

  enum Columns
  {
    Conformer = 0,
    Energy,
    Prob
  };

  GlobalSearch::Structure* currentStructure();

public slots:
  void lockGUI();
  void readSettings(const QString& filename = "");
  void writeSettings(const QString& filename = "");
  void updateGUI();
  void generateConformers();
  void fillForceFieldCombo();
  void updateStructureList();
  void updateConformerTable();
  void updateForceField(const QString& s = "");
  void selectStructure(const QString& text);
  void conformerChanged(int row, int, int oldrow, int);
  void calculateNumberOfConformers(bool isSystematic);

signals:
  void conformerGenerationStarting();
  void conformerGenerationDone();
  void conformersChanged();

private slots:
  void enableGenerateButton() { ui.push_generate->setEnabled(true); };
  void disableGenerateButton() { ui.push_generate->setEnabled(false); };

private:
  Ui::Tab_Conformers ui;
  OpenBabel::OBForceField* m_ff;
  std::vector<std::string> m_forceFieldList;
  QMutex m_ffMutex;

  void generateConformers_(GlobalSearch::Structure* mol);
};
}

#endif
