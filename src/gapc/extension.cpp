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

#include <gapc/extension.h>

#include <globalsearch/structure.h>

#include <avogadro/primitive.h>
#include <avogadro/molecule.h>
#include <avogadro/atom.h>
#include <avogadro/glwidget.h>

#include <openbabel/generic.h>
#include <openbabel/mol.h>

#include <QAction>
#include <QWidget>
#include <QMessageBox>
#include <QDebug>

using namespace Avogadro;
using namespace GlobalSearch;

namespace GAPC {

  GAPCExtension::GAPCExtension(QObject *parent) : Extension(parent),
                                                  m_dialog(0),
                                                  m_molecule(NULL)
  {
    QAction *action = new QAction( this );
    action->setSeparator(true);
    m_actions.append( action );

    action = new QAction(this);
    action->setText(tr("&Evolutionary Cluster Search..."));
    m_actions.append(action);
  }

  GAPCExtension::~GAPCExtension()
  {
  }

  QList<QAction *> GAPCExtension::actions() const
  {
    return m_actions;
  }

  void GAPCExtension::writeSettings(QSettings &settings) const
  {
    Extension::writeSettings(settings);
    if (m_dialog) {
      m_dialog->writeSettings();
    }
  }

  void GAPCExtension::readSettings(QSettings &settings)
  {
    Extension::readSettings(settings);
    if (m_dialog) {
      m_dialog->readSettings();
    }
  }

  QString GAPCExtension::menuPath(QAction *) const
  {
    return tr("E&xtensions");
  }

  void GAPCExtension::setMolecule(Molecule *molecule)
  {
    m_molecule = molecule;
    if (m_dialog) {
      m_dialog->setMolecule(molecule);
    }
  }

  void GAPCExtension::reemitMoleculeChanged(Structure *s) {
    // Make copy of s to pass to editor
    GlobalSearch::Structure *newS = new GlobalSearch::Structure (*s);
    // Reset filename to something unique
    newS->setFileName(s->fileName() + "/usermodified.cml");

    emit moleculeChanged(newS, Extension::DeleteOld);
  }

  QUndoCommand* GAPCExtension::performAction( QAction *, GLWidget *widget )
  {
    if (m_molecule == NULL) {
      return NULL;
    }

    if (!m_dialog) {
      m_dialog = new GAPCDialog(widget, qobject_cast<QWidget*>(parent()));
      m_dialog->setMolecule(m_molecule);
      // Allow setting of the molecule from within the dialog:
      connect(m_dialog, SIGNAL(moleculeChanged(GlobalSearch::Structure*)),
              this, SLOT(reemitMoleculeChanged(GlobalSearch::Structure*)));
    }
    m_dialog->show();
    return NULL;
  }
}

Q_EXPORT_PLUGIN2(gapcextension, GAPC::GAPCExtensionFactory)
