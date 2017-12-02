/**********************************************************************
  TabInit - Parameter for initializing the search

  Copyright (C) 2009-2011 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <gapc/ui/tab_init.h>

#include <gapc/gapc.h>
#include <globalsearch/macros.h>

#include <openbabel/mol.h> // for etab

#include <QSettings>
#include <QTextStream>

#include "dialog.h"

namespace GAPC {

TabInit::TabInit(GAPCDialog* parent, OptGAPC* p) : AbstractTab(parent, p)
{
  ui.setupUi(m_tab_widget);

  // param connections
  connect(ui.spin_minIAD, SIGNAL(valueChanged(double)), this,
          SLOT(updateParams()));
  connect(ui.spin_maxIAD, SIGNAL(valueChanged(double)), this,
          SLOT(updateParams()));

  // composition connections
  connect(ui.edit_composition, SIGNAL(textChanged(QString)), this,
          SLOT(getComposition(QString)));
  connect(ui.edit_composition, SIGNAL(editingFinished()), this,
          SLOT(updateComposition()));

  initialize();
}

TabInit::~TabInit()
{
}

void TabInit::writeSettings(const QString& filename)
{
  SETTINGS(filename);

  OptGAPC* gapc = qobject_cast<OptGAPC*>(m_opt);

  settings->beginGroup("gapc/init/");

  const int version = 1;
  settings->setValue("version", version);
  settings->setValue("minIAD", gapc->minIAD);
  settings->setValue("maxIAD", gapc->maxIAD);

  // Composition
  // We only want to save POTCAR info and Composition to the resume
  // file, not the main config file, so only dump the data here if
  // we are given a filename and it contains the string
  // "xtalopt.state"
  if (!filename.isEmpty() && filename.contains("gapc.state")) {
    settings->beginWriteArray("composition");
    QList<uint> keys = gapc->comp.core.keys();
    for (int i = 0; i < keys.size(); i++) {
      settings->setArrayIndex(i);
      settings->setValue("atomicNumber", keys.at(i));
      settings->setValue("quantity", gapc->comp.core.value(keys.at(i)));
    }
    settings->endArray();
  }

  settings->endGroup();

  DESTROY_SETTINGS(filename);
}

void TabInit::readSettings(const QString& filename)
{
  SETTINGS(filename);

  OptGAPC* gapc = qobject_cast<OptGAPC*>(m_opt);

  settings->beginGroup("gapc/init/");
  int loadedVersion = settings->value("version", 0).toInt();
  ui.spin_minIAD->setValue(settings->value("minIAD", 0.8).toDouble());
  ui.spin_maxIAD->setValue(settings->value("maxIAD", 3.0).toDouble());

  // Composition
  if (!filename.isEmpty()) {
    int size = settings->beginReadArray("composition");
    gapc->comp.core = QHash<uint, uint>();
    for (int i = 0; i < size; i++) {
      settings->setArrayIndex(i);
      uint atomicNum, quant;
      atomicNum = settings->value("atomicNumber").toUInt();
      quant = settings->value("quantity").toUInt();
      gapc->comp.core.insert(atomicNum, quant);
    }
    settings->endArray();
  }

  settings->endGroup();

  // Update config data
  switch (loadedVersion) {
    case 0:
    case 1:
    default:
      break;
  }

  // Enact changesSetup templates
  updateParams();
}

void TabInit::updateGUI()
{
  OptGAPC* gapc = qobject_cast<OptGAPC*>(m_opt);

  ui.spin_minIAD->setValue(gapc->minIAD);
  ui.spin_maxIAD->setValue(gapc->maxIAD);

  updateComposition();
}

void TabInit::lockGUI()
{
  ui.edit_composition->setDisabled(true);
}

void TabInit::getComposition(const QString& str)
{
  OptGAPC* gapc = qobject_cast<OptGAPC*>(m_opt);

  QHash<uint, uint> comp;
  QString symbol;
  uint atomicNum;
  uint quantity;
  QStringList symbolList;
  QStringList quantityList;

  // Parse numbers between letters
  symbolList = str.split(QRegExp("[0-9]"), QString::SkipEmptyParts);
  // Parse letters between numbers
  quantityList = str.split(QRegExp("[A-Z,a-z]"), QString::SkipEmptyParts);

  // Use the shorter of the lists for the length
  uint length = (symbolList.size() < quantityList.size()) ? symbolList.size()
                                                          : quantityList.size();

  if (length == 0) {
    ui.list_composition->clear();
    gapc->comp.core.clear();
    return;
  }

  // Build hash
  for (uint i = 0; i < length; i++) {
    symbol = symbolList.at(i);
    quantity = quantityList.at(i).toUInt();

    atomicNum =
      OpenBabel::etab.GetAtomicNum(symbol.trimmed().toStdString().c_str());

    // Validate symbol
    if (!atomicNum)
      continue; // Invalid symbol entered

    // Add to hash
    if (!comp.keys().contains(atomicNum))
      comp[atomicNum] = 0; // initialize if needed
    comp[atomicNum] += quantity;
  }

  // Dump hash into list
  ui.list_composition->clear();
  QList<uint> keys = comp.keys();
  qSort(keys);
  QString line("%1=%2 x %3");
  for (int i = 0; i < keys.size(); i++) {
    atomicNum = keys.at(i);
    quantity = comp[atomicNum];
    symbol = OpenBabel::etab.GetSymbol(atomicNum);
    new QListWidgetItem(line.arg(atomicNum, 3).arg(symbol, 2).arg(quantity),
                        ui.list_composition);
  }

  // Save hash
  gapc->comp.core = comp;
}

void TabInit::updateComposition()
{
  OptGAPC* gapc = qobject_cast<OptGAPC*>(m_opt);

  QList<uint> keys = gapc->comp.core.keys();
  qSort(keys);
  QString tmp;
  QTextStream str(&tmp);
  for (int i = 0; i < keys.size(); i++) {
    uint q = gapc->comp.core.value(keys.at(i));
    str << OpenBabel::etab.GetSymbol(keys.at(i)) << q << " ";
  }
  ui.edit_composition->setText(tmp.trimmed());
}

void TabInit::updateParams()
{
  OptGAPC* gapc = qobject_cast<OptGAPC*>(m_opt);

  gapc->minIAD = ui.spin_minIAD->value();
  gapc->maxIAD = ui.spin_maxIAD->value();
}
}
