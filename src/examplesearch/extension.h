/**********************************************************************
  ExampleSearch

  Copyright (C) 2012 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifndef EXAMPLESEARCHEXTENSION_H
#define EXAMPLESEARCHEXTENSION_H

#include <avogadro/extension.h>

#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtCore/QString>

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
      ExampleSearchExtension(QObject *parent=0);
      virtual ~ExampleSearchExtension();

      //! Perform Action
      virtual QList<QAction *> actions() const;
      virtual QUndoCommand* performAction(QAction *action, Avogadro::GLWidget *widget);
      virtual QString menuPath(QAction *action) const;
      void writeSettings(QSettings &settings) const;
      void readSettings(QSettings &settings);

   public slots:
      void reemitMoleculeChanged(GlobalSearch::Structure *structure);

    private:
      QList<QAction *> m_actions;
      ExampleSearchDialog *m_dialog;
  };

  // Workaround for Avogadro bug:
  using Avogadro::Plugin;

  class ExampleSearchExtensionFactory :
      public QObject,
      public Avogadro::PluginFactory
  {
      Q_OBJECT
      Q_INTERFACES(Avogadro::PluginFactory)
      AVOGADRO_EXTENSION_FACTORY(ExampleSearchExtension)
  };


} // end namespace ExampleSearch

#endif
