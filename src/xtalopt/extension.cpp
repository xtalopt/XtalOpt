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

#include <xtalopt/extension.h>

#include <xtalopt/structures/molecularxtal.h>
#include <xtalopt/structures/submolecule.h>
#include <xtalopt/structures/xtal.h>
#include <xtalopt/ui/dialog.h>

#include <globalsearch/macros.h>

#include <avogadro/atom.h>
#include <avogadro/glwidget.h>
#include <avogadro/molecule.h>
#include <avogadro/primitive.h>
#include <avogadro/tool.h>
#include <avogadro/toolgroup.h>

#include <openbabel/generic.h>
#include <openbabel/mol.h>

#include <QtGui/QAction>
#include <QtGui/QWidget>
#include <QtGui/QMessageBox>

#include <QtCore/QDebug>

using namespace Avogadro;
using namespace GlobalSearch;

namespace XtalOpt {

  XtalOptExtension::XtalOptExtension(QObject *parent) : Extension(parent),
                                                        m_dialog(0),
                                                        m_molecule(NULL)
  {
    QAction *action = new QAction( this );
    action->setSeparator(true);
    m_actions.append( action );

    action = new QAction(this);
    action->setText(tr("&XtalOpt"));
    action->setIcon(
          QIcon(QString::fromUtf8(":/xtalopt/images/xtalopt-logo.png")));
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

  void XtalOptExtension::reemitMoleculeChanged(Structure *s) {
    // Make copy of Xtal to pass to editor
    Xtal *newXtal = NULL;
    MolecularXtal *newMXtal = NULL;
    if (MolecularXtal *sAsMXtal = qobject_cast<MolecularXtal *>(s)) {
      newXtal = newMXtal = new MolecularXtal();
      *newMXtal = *sAsMXtal;
      // Clean up the submolecules
      newMXtal->wrapAtomsToCell();
      QList<SubMolecule*> subs = newMXtal->subMolecules();
      for (QList<SubMolecule*>::iterator it = subs.begin(),
           it_end = subs.end(); it != it_end; ++it) {
        (*it)->makeCoherent();
      }
    }
    else {
      newXtal = new Xtal (*qobject_cast<Xtal*>(s));
    }
    // Reset filename to something unique
    newXtal->setFileName(s->fileName() + "/usermodified.cml");

    // Check for weirdness
    if (newXtal->numAtoms() != 0 && !newXtal->atom(0)) {
      qWarning() << "XtalOptExtension::reemitMoleculeChanged: Molecule is invalid (bad atoms) -- not sending to GLWidget";
      return;
    }
    if (GS_ISNAN(newXtal->getA()) ||
        GS_ISNAN(newXtal->getB()) ||
        GS_ISNAN(newXtal->getC()) ||
        GS_ISNAN(newXtal->getAlpha()) ||
        GS_ISNAN(newXtal->getBeta()) ||
        GS_ISNAN(newXtal->getGamma())) {
      qWarning() << "XtalOptExtension::reemitMoleculeChanged: Molecule is invalid (cell param is nan) -- not sending to GLWidget";
      return;
    }

    emit moleculeChanged(newXtal, Extension::DeleteOld);

    // If the draw tool is currently selected, switch to navigate
    Avogadro::Tool *currentTool =
        GLWidget::current()->toolGroup()->activeTool();
    if (currentTool->identifier().compare("Draw") == 0) {
      GLWidget::current()->toolGroup()->setActiveTool("Navigate");
    }
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
      connect(m_dialog, SIGNAL(moleculeChanged(GlobalSearch::Structure*)),
              this, SLOT(reemitMoleculeChanged(GlobalSearch::Structure*)));
    }
    m_dialog->show();
    return NULL;
  }
} // end namespace XtalOpt

Q_EXPORT_PLUGIN2(xtaloptextension, XtalOpt::XtalOptExtensionFactory)
