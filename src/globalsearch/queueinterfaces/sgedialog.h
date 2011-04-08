/**********************************************************************
  SgeConfigDialog -- Setup for remote SGE queues

  Copyright (C) 2011 by David Lonie

  This library is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as
  published by the Free Software Foundation; either version 2.1 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public icense for more details.
 ***********************************************************************/

#ifndef SGECONFIGDIALOG_H
#define SGECONFIGDIALOG_H

#ifdef ENABLE_SSH

//Doxygen should ignore this file:
/// @cond

#include <QtGui/QDialog>

namespace Ui {
  class SgeConfigDialog;
}

namespace GlobalSearch {
  class AbstractDialog;
  class OptBase;
  class SgeQueueInterface;

  class SgeConfigDialog : public QDialog
  {
    Q_OBJECT

  public:

    explicit SgeConfigDialog(AbstractDialog *parent,
                             OptBase *o,
                             SgeQueueInterface *p);
    virtual ~SgeConfigDialog();

  public slots:
    void updateGUI();

  protected slots:
    void accept();
    void reject();

  protected:
    OptBase *m_opt;
    SgeQueueInterface *m_sge;

  private:
    Ui::SgeConfigDialog *ui;

  };
}

/// @endcond
#endif // ENABLE_SSH
#endif // SGECONFIGDIALOG_H
