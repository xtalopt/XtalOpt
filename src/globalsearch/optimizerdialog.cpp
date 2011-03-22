/**********************************************************************
  OptimizerDialog - Generic optimizer configuration dialog

  Copyright (C) 2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

// Don't document this:
/// @cond

#include <globalsearch/optimizerdialog.h>

#include <globalsearch/optbase.h>
#include <globalsearch/optimizer.h>
#include <globalsearch/ui/abstractdialog.h>

#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QSpacerItem>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QVBoxLayout>

#include <QtCore/QObject>

namespace GlobalSearch {

  OptimizerConfigDialog::OptimizerConfigDialog
  (AbstractDialog *parent, OptBase *opt, Optimizer *o)
    : QDialog(parent),
      m_opt(opt),
      m_optimizer(o),
      m_lineedit(0)
  {
    QVBoxLayout *vlayout = new QVBoxLayout(this);

    QLabel *label = new QLabel
      (tr("Local path to %1 executable "
          "(only needed when using local queue interface):")
       .arg(m_optimizer->m_idString), this);
    vlayout->addWidget(label);

    m_lineedit = new QLineEdit(this);
    vlayout->addWidget(m_lineedit);

    QSpacerItem *spacer = new QSpacerItem
      (10,10, QSizePolicy::Minimum, QSizePolicy::Expanding);
    vlayout->addItem(spacer);

    QDialogButtonBox *bbox = new QDialogButtonBox(this);
    bbox->setStandardButtons(QDialogButtonBox::Ok |
                             QDialogButtonBox::Cancel );
    vlayout->addWidget(bbox);

    setLayout(vlayout);

    connect(bbox, SIGNAL(accepted()),
            this, SLOT(updateState()));
    connect(bbox, SIGNAL(accepted()),
            this, SLOT(close()));
    connect(bbox, SIGNAL(rejected()),
            this, SLOT(updateGUI()));
    connect(bbox, SIGNAL(rejected()),
            this, SLOT(close()));
  }

  void OptimizerConfigDialog::updateState()
  {
    m_optimizer->m_localRunCommand = m_lineedit->text();
  }

  void OptimizerConfigDialog::updateGUI()
  {
    m_lineedit->setText(m_optimizer->m_localRunCommand);
  }

} // end namespace GlobalSearch

/// @endcond
