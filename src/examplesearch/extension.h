/**********************************************************************
  ExampleSearch

  Copyright (C) 2012 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef EXAMPLESEARCHEXTENSION_H
#define EXAMPLESEARCHEXTENSION_H

#include <avogadro/extension.h>

#include <QList>
#include <QObject>
#include <QString>

class QUndoCommand;

namespace GlobalSearch {
class Structure;
}

namespace ExampleSearch {
class ExampleSearchDialog;

class ExampleSearchExtension : public Avogadro::Extension
{
  Q_OBJECT
  AVOGADRO_EXTENSION("ExampleSearch", tr("Example Search"),
                     tr("Example of a structure search Avogadro extension "
                        "using libglobalsearch"))

public:
  ExampleSearchExtension(QObject* parent = 0);
  virtual ~ExampleSearchExtension();

  //! Perform Action
  virtual QList<QAction*> actions() const;
  virtual QUndoCommand* performAction(QAction* action,
                                      Avogadro::GLWidget* widget);
  virtual QString menuPath(QAction* action) const;
  void writeSettings(QSettings& settings) const;
  void readSettings(QSettings& settings);

public slots:
  void reemitMoleculeChanged(GlobalSearch::Structure* structure);

private:
  QList<QAction*> m_actions;
  ExampleSearchDialog* m_dialog;
};

// Workaround for Avogadro bug:
using Avogadro::Plugin;

class ExampleSearchExtensionFactory : public QObject,
                                      public Avogadro::PluginFactory
{
  Q_OBJECT
  Q_INTERFACES(Avogadro::PluginFactory)
  AVOGADRO_EXTENSION_FACTORY(ExampleSearchExtension)
};

} // end namespace ExampleSearch

#endif
