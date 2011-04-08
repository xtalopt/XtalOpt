/**********************************************************************
  XtalOpt - Tools for advanced crystal optimization

  Copyright (C) 2009-2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifndef XTALOPTEXTENSION_H
#define XTALOPTEXTENSION_H

#include <xtalopt/ui/dialog.h>

#include <avogadro/extension.h>

#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtCore/QString>

#include <QtGui/QUndoCommand>

namespace XtalOpt {

  class XtalOptExtension : public Avogadro::Extension
  {
    Q_OBJECT
    AVOGADRO_EXTENSION("XtalOpt", tr("Crystal Optimization"),
                       tr("Tools for advanced crystal optimization"))

    public:
      //! Constructor
      XtalOptExtension(QObject *parent=0);
      //! Deconstructor
      virtual ~XtalOptExtension();

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
      XtalOptDialog *m_dialog;
      Avogadro::Molecule *m_molecule;
  };

  // Workaround for Avogadro bug:
  using Avogadro::Plugin;

  class XtalOptExtensionFactory : public QObject, public Avogadro::PluginFactory
  {
      Q_OBJECT
      Q_INTERFACES(Avogadro::PluginFactory)
      AVOGADRO_EXTENSION_FACTORY(XtalOptExtension)
  };


} // end namespace XtalOpt

#endif
