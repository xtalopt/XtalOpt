/**********************************************************************
  LocalQueueInterfaceConfigDialog

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

#include <globalsearch/queueinterfaces/localdialog.h>

#include <globalsearch/optbase.h>
#include <globalsearch/queueinterfaces/local.h>
#include <globalsearch/ui/abstractdialog.h>

#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QSpacerItem>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QVBoxLayout>

#include <QtCore/QObject>

namespace GlobalSearch {

  LocalQueueInterfaceConfigDialog::LocalQueueInterfaceConfigDialog
  (AbstractDialog *parent, OptBase *opt, LocalQueueInterface *o)
    : QDialog(parent),
      m_opt(opt),
      m_queueInterface(o),
      m_lineedit(0)
  {
    QVBoxLayout *vlayout = new QVBoxLayout(this);

    QLabel *label = new QLabel
      (tr("Local working directory:"), this);
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
            this, SLOT(accept()));
    connect(bbox, SIGNAL(rejected()),
            this, SLOT(reject()));
  }

  void LocalQueueInterfaceConfigDialog::accept()
  {
    m_opt->filePath = m_lineedit->text();
    QDialog::accept();
    this->close();
  }

  void LocalQueueInterfaceConfigDialog::reject()
  {
    updateGUI();
    QDialog::reject();
    this->close();
  }

  void LocalQueueInterfaceConfigDialog::updateGUI()
  {
    m_lineedit->setText(m_opt->filePath);
  }

} // end namespace GlobalSearch

/// @endcond
