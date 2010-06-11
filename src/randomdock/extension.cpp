/**********************************************************************
  RandomDock - Random Docking Search

  Copyright (C) 2009-2010 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include <randomdock/extension.h>

#include <randomdock/ui/dialog.h>
#include <globalsearch/structure.h>

#include <avogadro/molecule.h>
#include <avogadro/glwidget.h>

#include <QAction>
#include <QWidget>
#include <QMessageBox>
#include <QDebug>

using namespace std;
using namespace OpenBabel;
using namespace Eigen;
using namespace Avogadro;

namespace RandomDock {

  RandomDockExtension::RandomDockExtension(QObject *parent) :
    Extension(parent),
    m_dialog(0),
    m_molecule(NULL)
  {
    QAction *action = new QAction( this );
    action->setSeparator(true);
    m_actions.append( action );

    action = new QAction(this);
    action->setText(tr("&Random Docking Search..."));
    m_actions.append(action);
  }

  RandomDockExtension::~RandomDockExtension()
  {
  }

  QList<QAction *> RandomDockExtension::actions() const
  {
    return m_actions;
  }

  void RandomDockExtension::writeSettings(QSettings &settings) const
  {
    Extension::writeSettings(settings);
    if (m_dialog) {
      m_dialog->writeSettings();
    }
  }

  void RandomDockExtension::readSettings(QSettings &settings)
  {
    Extension::readSettings(settings);
    if (m_dialog) {
      m_dialog->readSettings();
    }
  }

  QString RandomDockExtension::menuPath(QAction *) const
  {
    return tr("E&xtensions");
  }

  void RandomDockExtension::setMolecule(Molecule *molecule)
  {
    m_molecule = molecule;
    if (m_dialog) {
      m_dialog->setMolecule(molecule);
    }
  }

  void RandomDockExtension::reemitMoleculeChanged(Structure *mol) {
    // Check for weirdness
    if (mol->numAtoms() != 0) {
      if (!mol->atom(0)) {
        qDebug() << "RandomDockExtension::reemitMoleculeChanged: Molecule is invalid -- not sending to GLWidget";
        return;
      }
    }
    emit moleculeChanged(mol, Extension::KeepOld);
  }

  QUndoCommand* RandomDockExtension::performAction( QAction *, GLWidget *widget )
  {
    if (m_molecule == NULL) {
      return NULL;
    }

    if (!m_dialog) {
      m_dialog = new RandomDockDialog(widget, qobject_cast<QWidget*>(parent()));
      m_dialog->setMolecule(m_molecule);
      // Allow setting of the molecule from within the dialog:
      connect(m_dialog, SIGNAL(moleculeChanged(Structure*)),
              this, SLOT(reemitMoleculeChanged(Structure*)));
    }
    m_dialog->show();
    return NULL;
  }
} // end namespace RandomDock

//#include "extension.moc"

Q_EXPORT_PLUGIN2(randomdockextension, RandomDock::RandomDockExtensionFactory)
