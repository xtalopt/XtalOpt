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

#ifndef GAPCEXTENSION_H
#define GAPCEXTENSION_H

#include <gapc/ui/dialog.h>

#include <avogadro/extension.h>

#include <QList>
#include <QObject>
#include <QString>
#include <QUndoCommand>

namespace GAPC {

class GAPCExtension : public Avogadro::Extension
{
  Q_OBJECT
  AVOGADRO_EXTENSION("GAPC", tr("Evolutionary Cluster Search"),
                     tr("Search for a stable cluster configuration using "
                        "evolutionary techniques."))

public:
  //! Constructor
  GAPCExtension(QObject* parent = 0);
  //! Deconstructor
  virtual ~GAPCExtension();

  //! Perform Action
  virtual QList<QAction*> actions() const;
  virtual QUndoCommand* performAction(QAction* action,
                                      Avogadro::GLWidget* widget);
  virtual QString menuPath(QAction* action) const;
  virtual void setMolecule(Avogadro::Molecule* molecule);
  void writeSettings(QSettings& settings) const;
  void readSettings(QSettings& settings);

public slots:
  void reemitMoleculeChanged(GlobalSearch::Structure* s);

private:
  QList<QAction*> m_actions;
  GAPCDialog* m_dialog;
  Avogadro::Molecule* m_molecule;
};

// workaround for Avogadro bug:
using Avogadro::Plugin;

class GAPCExtensionFactory : public QObject, public Avogadro::PluginFactory
{
  Q_OBJECT
  Q_INTERFACES(Avogadro::PluginFactory)
  AVOGADRO_EXTENSION_FACTORY(GAPCExtension)
};
}

#endif
