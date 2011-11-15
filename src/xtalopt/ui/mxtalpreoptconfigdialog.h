/**********************************************************************
  MXtalPreoptConfigDialog

  Copyright (C) 2012 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifndef MXTALPREOPTCONFIGDIALOG_H
#define MXTALPREOPTCONFIGDIALOG_H

#include <QtGui/QDialog>

namespace Ui
{
class MXtalPreoptConfigDialog;
}

namespace XtalOpt
{

class XtalOpt;

class MXtalPreoptConfigDialog : public QDialog
{
  Q_OBJECT

public:
  explicit MXtalPreoptConfigDialog(XtalOpt *xtalopt, QWidget *parent = 0);
  ~MXtalPreoptConfigDialog();

public slots:
  void accept();

private:
  Ui::MXtalPreoptConfigDialog *ui;
  XtalOpt *m_opt;
};

}
#endif // MXTALPREOPTCONFIGDIALOG_H
