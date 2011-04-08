/**********************************************************************
  GAPC -- A genetic algorithm for protected clusters

  Copyright (C) 2010-2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifndef GAPCEXTENSION_H
#define GAPCEXTENSION_H

#include <gapc/ui/dialog.h>

#include <avogadro/extension.h>

#include <QObject>
#include <QList>
#include <QString>
#include <QUndoCommand>

namespace GAPC {

  class GAPCExtension : public Avogadro::Extension
  {
    Q_OBJECT
    AVOGADRO_EXTENSION("GAPC", tr("Evolutionary Cluster Search"),
                       tr("Search for a stable cluster configuration using evolutionary techniques."))

    public:
      //! Constructor
      GAPCExtension(QObject *parent=0);
      //! Deconstructor
      virtual ~GAPCExtension();

      //! Perform Action
      virtual QList<QAction *> actions() const;
      virtual QUndoCommand* performAction(QAction *action, Avogadro::GLWidget *widget);
      virtual QString menuPath(QAction *action) const;
      virtual void setMolecule(Avogadro::Molecule *molecule);
      void writeSettings(QSettings &settings) const;
      void readSettings(QSettings &settings);

   public slots:
      void reemitMoleculeChanged(GlobalSearch::Structure *s);

    private:
      QList<QAction *> m_actions;
      GAPCDialog *m_dialog;
      Avogadro::Molecule *m_molecule;
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
