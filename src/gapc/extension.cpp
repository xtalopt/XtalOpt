/**********************************************************************
  GAPC -- A genetic algorithm for protected clusters

  Copyright (C) 2010-2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <gapc/extension.h>

#include <globalsearch/structure.h>

#include <avogadro/atom.h>
#include <avogadro/glwidget.h>
#include <avogadro/molecule.h>
#include <avogadro/primitive.h>

#include <openbabel/generic.h>
#include <openbabel/mol.h>

#include <QAction>
#include <QDebug>
#include <QMessageBox>
#include <QWidget>

using namespace Avogadro;
using namespace GlobalSearch;

namespace GAPC {

GAPCExtension::GAPCExtension(QObject* parent)
  : Extension(parent), m_dialog(0), m_molecule(NULL)
{
  QAction* action = new QAction(this);
  action->setSeparator(true);
  m_actions.append(action);

  action = new QAction(this);
  action->setText(tr("&Evolutionary Cluster Search..."));
  m_actions.append(action);
}

GAPCExtension::~GAPCExtension()
{
}

QList<QAction*> GAPCExtension::actions() const
{
  return m_actions;
}

void GAPCExtension::writeSettings(QSettings& settings) const
{
  Extension::writeSettings(settings);
  if (m_dialog) {
    m_dialog->writeSettings();
  }
}

void GAPCExtension::readSettings(QSettings& settings)
{
  Extension::readSettings(settings);
  if (m_dialog) {
    m_dialog->readSettings();
  }
}

QString GAPCExtension::menuPath(QAction*) const
{
  return tr("E&xtensions");
}

void GAPCExtension::setMolecule(Molecule* molecule)
{
  m_molecule = molecule;
  if (m_dialog) {
    m_dialog->setMolecule(molecule);
  }
}

void GAPCExtension::reemitMoleculeChanged(Structure* s)
{
  // Make copy of s to pass to editor
  GlobalSearch::Structure* newS = new GlobalSearch::Structure(*s);
  // Reset filename to something unique
  newS->setFileName(s->fileName() + "/usermodified.cml");

  emit moleculeChanged(newS, Extension::DeleteOld);
}

QUndoCommand* GAPCExtension::performAction(QAction*, GLWidget* widget)
{
  if (m_molecule == NULL) {
    return NULL;
  }

  if (!m_dialog) {
    m_dialog = new GAPCDialog(widget, qobject_cast<QWidget*>(parent()));
    m_dialog->setMolecule(m_molecule);
    // Allow setting of the molecule from within the dialog:
    connect(m_dialog, SIGNAL(moleculeChanged(GlobalSearch::Structure*)), this,
            SLOT(reemitMoleculeChanged(GlobalSearch::Structure*)));
  }
  m_dialog->show();
  return NULL;
}
}

Q_EXPORT_PLUGIN2(gapcextension, GAPC::GAPCExtensionFactory)
