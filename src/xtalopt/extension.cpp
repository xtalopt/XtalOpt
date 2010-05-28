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

#include "extension.h"

#include "ui/dialog.h"
#include "../generic/xtal.h"

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

using namespace std;
using namespace OpenBabel;
using namespace Eigen;

namespace Avogadro {

  XtalOptExtension::XtalOptExtension(QObject *parent) : Extension(parent),
                                                        m_dialog(0),
                                                        m_molecule(NULL)
  {
    QAction *action = new QAction( this );
    action->setSeparator(true);
    m_actions.append( action );

    action = new QAction(this);
    action->setText(tr("&Crystal Optimization..."));
    m_actions.append(action);
  }

  XtalOptExtension::~XtalOptExtension()
  {
  }

  QList<QAction *> XtalOptExtension::actions() const
  {
    return m_actions;
  }

  void XtalOptExtension::writeSettings(QSettings &settings) const
  {
    Extension::writeSettings(settings);
    if (m_dialog) {
      m_dialog->writeSettings();
    }
  }

  void XtalOptExtension::readSettings(QSettings &settings)
  {
    Extension::readSettings(settings);
    if (m_dialog) {
      m_dialog->readSettings();
    }
  }

  QString XtalOptExtension::menuPath(QAction *) const
  {
    return tr("E&xtensions");
  }

  void XtalOptExtension::setMolecule(Molecule *molecule)
  {
    m_molecule = molecule;
    if (m_dialog) {
      m_dialog->setMolecule(molecule);
    }
  }

  void XtalOptExtension::reemitMoleculeChanged(Xtal* xtal) {
    //qDebug() << "XtalOptExtension::reemitMoleculeChanged( " << xtal << " ) called.";
    // Check for weirdness
    if (xtal->numAtoms() != 0) {
      if (!xtal->atom(0)) {
        qDebug() << "XtalOptExtension::reemitMoleculeChanged: Molecule is invalid -- not sending to GLWidget";
        return;
      }
    }
    emit moleculeChanged(xtal, Extension::KeepOld);
  }

  QUndoCommand* XtalOptExtension::performAction( QAction *, GLWidget *widget )
  {
    if (m_molecule == NULL) {
      return NULL;
    }

    widget->setRenderUnitCellAxes(true);

    if (!m_dialog) {
      m_dialog = new XtalOptDialog(widget, qobject_cast<QWidget*>(parent()));
      m_dialog->setMolecule(m_molecule);
      // Allow setting of the molecule from within the dialog:
      connect(m_dialog, SIGNAL(moleculeChanged(Xtal*)),
              this, SLOT(reemitMoleculeChanged(Xtal*)));
    }
    m_dialog->show();
    return NULL;
  }
} // end namespace Avogadro

//#include "extension.moc"

Q_EXPORT_PLUGIN2(xtaloptextension, Avogadro::XtalOptExtensionFactory)
