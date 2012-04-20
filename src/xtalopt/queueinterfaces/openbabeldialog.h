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
#ifndef OPENBABELQUEUEINTERFACECONFIGDIALOG_H
#define OPENBABELQUEUEINTERFACECONFIGDIALOG_H

#include <QtGui/QDialog>

class QLineEdit;

namespace GlobalSearch {
  class AbstractDialog;
  class OptBase;
}

namespace XtalOpt {
class OpenBabelQueueInterface;

// Basic input dialog for Internal QueueInterfaces
class OpenBabelQueueInterfaceConfigDialog : public QDialog
{
  Q_OBJECT;
public:
  OpenBabelQueueInterfaceConfigDialog(GlobalSearch::AbstractDialog *parent,
                                      GlobalSearch::OptBase *opt,
                                      OpenBabelQueueInterface *qi);

public slots:
  void accept();
  void reject();
  void updateGUI();

protected:
  GlobalSearch::OptBase *m_opt;
  OpenBabelQueueInterface *m_queueInterface;
  QLineEdit *m_edit_workdir;
  QLineEdit *m_edit_description;

};

} // end namespace XtalOpt

#endif
/// @endcond
