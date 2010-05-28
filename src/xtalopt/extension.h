/**********************************************************************
  XtalOpt - Tools for advanced crystal optimization

  Copyright (C) 2009-2010 by David C. Lonie

  This file is part of the Avogadro molecular editor project.
  For more information, see <http://avogadro.openmolecules.net/>

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

#include "ui/dialog.h"

#include <avogadro/extension.h>

#include <QObject>
#include <QList>
#include <QString>
#include <QUndoCommand>

namespace Avogadro {

 class XtalOptExtension : public Extension
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
      virtual QUndoCommand* performAction(QAction *action, GLWidget *widget);
      virtual QString menuPath(QAction *action) const;
      virtual void setMolecule(Molecule *molecule);
      void writeSettings(QSettings &settings) const;
      void readSettings(QSettings &settings);

   public slots:

      void reemitMoleculeChanged(Xtal* xtal);

    private:
      QList<QAction *> m_actions;
      XtalOptDialog *m_dialog;
      Molecule *m_molecule;
  };

  class XtalOptExtensionFactory : public QObject, public PluginFactory
  {
      Q_OBJECT
      Q_INTERFACES(Avogadro::PluginFactory)
      AVOGADRO_EXTENSION_FACTORY(XtalOptExtension)
  };


} // end namespace Avogadro

#endif
