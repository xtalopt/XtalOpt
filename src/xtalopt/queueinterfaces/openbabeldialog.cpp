/**********************************************************************
  OpenBabelQueueInterfaceConfigDialog

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

#include <xtalopt/queueinterfaces/openbabeldialog.h>

#include <xtalopt/queueinterfaces/openbabel.h>

#include <globalsearch/optbase.h>
#include <globalsearch/ui/abstractdialog.h>

#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QSpacerItem>
#include <QtGui/QVBoxLayout>

#include <QtCore/QObject>

using namespace GlobalSearch;

namespace XtalOpt {

  OpenBabelQueueInterfaceConfigDialog::OpenBabelQueueInterfaceConfigDialog
  (AbstractDialog *parent, OptBase *opt, OpenBabelQueueInterface *o)
    : QDialog(parent),
      m_opt(opt),
      m_queueInterface(o),
      m_edit_workdir(0),
      m_edit_description(0)
  {
    QVBoxLayout *vlayout = new QVBoxLayout(this);

    // Create workdir prompt
    QHBoxLayout *workdir_layout = new QHBoxLayout();

    QLabel *label = new QLabel
      (tr("Local working directory:"), this);
    workdir_layout->addWidget(label);

    m_edit_workdir = new QLineEdit(this);
    workdir_layout->addWidget(m_edit_workdir);

    vlayout->addItem(workdir_layout);

    // Create description prompt
    QHBoxLayout *desc_layout = new QHBoxLayout();

    label = new QLabel
      (tr("Search description:"), this);
    desc_layout->addWidget(label);

    m_edit_description = new QLineEdit(this);
    desc_layout->addWidget(m_edit_description);

    vlayout->addItem(desc_layout);

    // Add spacer
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

  void OpenBabelQueueInterfaceConfigDialog::accept()
  {
    m_opt->filePath = m_edit_workdir->text().trimmed();
    m_opt->description = m_edit_description->text().trimmed();
    QDialog::accept();
    this->close();
  }

  void OpenBabelQueueInterfaceConfigDialog::reject()
  {
    updateGUI();
    QDialog::reject();
    this->close();
  }

  void OpenBabelQueueInterfaceConfigDialog::updateGUI()
  {
    m_edit_workdir->setText(m_opt->filePath);
    m_edit_description->setText(m_opt->description);
  }

} // end namespace XtalOpt

/// @endcond
