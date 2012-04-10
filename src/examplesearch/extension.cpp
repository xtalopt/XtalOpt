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

#include <examplesearch/extension.h>

#include <examplesearch/ui/dialog.h>

#include <globalsearch/structure.h>

#include <avogadro/molecule.h>
#include <avogadro/glwidget.h>

#include <QtCore/QDebug>

#include <QtGui/QAction>
#include <QtGui/QWidget>
#include <QtGui/QMessageBox>

using namespace std;
using namespace OpenBabel;
using namespace Eigen;
using namespace Avogadro;

namespace ExampleSearch {

  ExampleSearchExtension::ExampleSearchExtension(QObject *parent) :
    Extension(parent),
    m_dialog(0)
  {
    QAction *action = new QAction( this );
    action->setSeparator(true);
    m_actions.append( action );

    action = new QAction(this);
    // This is the name of the menu entry:
    action->setText(tr("&Example Search..."));
    m_actions.append(action);
  }

  ExampleSearchExtension::~ExampleSearchExtension()
  {
  }

  QList<QAction *> ExampleSearchExtension::actions() const
  {
    return m_actions;
  }

  void ExampleSearchExtension::writeSettings(QSettings &settings) const
  {
    Extension::writeSettings(settings);
    if (m_dialog) {
      m_dialog->writeSettings();
    }
  }

  void ExampleSearchExtension::readSettings(QSettings &settings)
  {
    Extension::readSettings(settings);
    if (m_dialog) {
      m_dialog->readSettings();
    }
  }

  QString ExampleSearchExtension::menuPath(QAction *) const
  {
    // This is the menu that the extension will appear in
    return tr("E&xtensions");
  }

  void ExampleSearchExtension::reemitMoleculeChanged(GlobalSearch::Structure *s) {
    // Make copy of s to pass to editor
    GlobalSearch::Structure *newS = new GlobalSearch::Structure (*s);
    // Reset filename to something unique
    newS->setFileName(s->fileName() + "/usermodified.cml");

    // Make any pre-vis cleanup changes you'd like here.

    emit moleculeChanged(newS, Extension::DeleteOld);
  }

  QUndoCommand* ExampleSearchExtension::performAction( QAction *, GLWidget *widget )
  {
    if (!m_dialog) {
      m_dialog = new ExampleSearchDialog(widget, qobject_cast<QWidget*>(parent()));
      // Allow setting of the molecule from within the dialog:
      connect(m_dialog, SIGNAL(moleculeChanged(GlobalSearch::Structure*)),
              this, SLOT(reemitMoleculeChanged(GlobalSearch::Structure*)));
    }
    m_dialog->show();
    return NULL;
  }
} // end namespace ExampleSearch

Q_EXPORT_PLUGIN2(examplesearchextension, ExampleSearch::ExampleSearchExtensionFactory)
