/**********************************************************************
  RandomDock - Random Docking Search

  Copyright (C) 2009-2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <randomdock/extension.h>

#include <globalsearch/structure.h>
#include <randomdock/ui/dialog.h>

#include <avogadro/glwidget.h>
#include <avogadro/molecule.h>

#include <QDebug>

#include <QAction>
#include <QMessageBox>
#include <QWidget>

using namespace std;
using namespace OpenBabel;
using namespace Eigen;
using namespace Avogadro;

namespace RandomDock {

RandomDockExtension::RandomDockExtension(QObject* parent)
  : Extension(parent), m_dialog(0), m_molecule(NULL)
{
  QAction* action = new QAction(this);
  action->setSeparator(true);
  m_actions.append(action);

  action = new QAction(this);
  action->setText(tr("&Random Docking Search..."));
  m_actions.append(action);
}

RandomDockExtension::~RandomDockExtension()
{
}

QList<QAction*> RandomDockExtension::actions() const
{
  return m_actions;
}

void RandomDockExtension::writeSettings(QSettings& settings) const
{
  Extension::writeSettings(settings);
  if (m_dialog) {
    m_dialog->writeSettings();
  }
}

void RandomDockExtension::readSettings(QSettings& settings)
{
  Extension::readSettings(settings);
  if (m_dialog) {
    m_dialog->readSettings();
  }
}

QString RandomDockExtension::menuPath(QAction*) const
{
  return tr("E&xtensions");
}

void RandomDockExtension::setMolecule(Molecule* molecule)
{
  m_molecule = molecule;
}

void RandomDockExtension::reemitMoleculeChanged(GlobalSearch::Structure* s)
{
  // Make copy of s to pass to editor
  GlobalSearch::Structure* newS = new GlobalSearch::Structure(*s);
  // Reset filename to something unique
  newS->setFileName(s->fileName() + "/usermodified.cml");

  // Check for weirdness
  if (newS->numAtoms() != 0) {
    if (!newS->atom(0)) {
      qDebug() << "RandomDockExtension::reemitMoleculeChanged: Molecule is "
                  "invalid -- not sending to GLWidget";
      return;
    }
  }
  emit moleculeChanged(newS, Extension::DeleteOld);
}

QUndoCommand* RandomDockExtension::performAction(QAction*, GLWidget* widget)
{
  if (m_molecule == NULL) {
    return NULL;
  }

  if (!m_dialog) {
    m_dialog = new RandomDockDialog(widget, qobject_cast<QWidget*>(parent()));
    // Allow setting of the molecule from within the dialog:
    connect(m_dialog, SIGNAL(moleculeChanged(GlobalSearch::Structure*)), this,
            SLOT(reemitMoleculeChanged(GlobalSearch::Structure*)));
  }
  m_dialog->show();
  return NULL;
}
} // end namespace RandomDock

Q_EXPORT_PLUGIN2(randomdockextension, RandomDock::RandomDockExtensionFactory)
