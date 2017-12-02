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

#ifndef RDEXTENSION_H
#define RDEXTENSION_H

#include <randomdock/ui/dialog.h>

#include <avogadro/extension.h>

#include <QList>
#include <QObject>
#include <QString>

#include <QUndoCommand>

namespace RandomDock {

class RandomDockExtension : public Avogadro::Extension
{
  Q_OBJECT
  AVOGADRO_EXTENSION("RandomDock", tr("Random Docking Search"),
                     tr("Tool to search for docking configurations."))

public:
  //! Constructor
  RandomDockExtension(QObject* parent = 0);
  //! Deconstructor
  virtual ~RandomDockExtension();

  //! Perform Action
  virtual QList<QAction*> actions() const;
  virtual QUndoCommand* performAction(QAction* action,
                                      Avogadro::GLWidget* widget);
  virtual QString menuPath(QAction* action) const;
  virtual void setMolecule(Avogadro::Molecule* molecule);
  void writeSettings(QSettings& settings) const;
  void readSettings(QSettings& settings);

public slots:
  void reemitMoleculeChanged(GlobalSearch::Structure* structure);

private:
  QList<QAction*> m_actions;
  RandomDockDialog* m_dialog;
  Avogadro::Molecule* m_molecule;
};

// Workaround for Avogadro bug:
using Avogadro::Plugin;

class RandomDockExtensionFactory : public QObject,
                                   public Avogadro::PluginFactory
{
  Q_OBJECT
  Q_INTERFACES(Avogadro::PluginFactory)
  AVOGADRO_EXTENSION_FACTORY(RandomDockExtension)
};

} // end namespace RandomDock

#endif
