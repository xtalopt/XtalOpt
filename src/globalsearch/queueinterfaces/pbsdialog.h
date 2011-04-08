/**********************************************************************
  PbsConfigDialog -- Setup for remote PBS queues

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

#ifndef PBSCONFIGDIALOG_H
#define PBSCONFIGDIALOG_H

#ifdef ENABLE_SSH

//Doxygen should ignore this file:
/// @cond

#include <QtGui/QDialog>

namespace Ui {
  class PbsConfigDialog;
}

namespace GlobalSearch {
  class AbstractDialog;
  class OptBase;
  class PbsQueueInterface;

  class PbsConfigDialog : public QDialog
  {
    Q_OBJECT

  public:

    explicit PbsConfigDialog(AbstractDialog *parent,
                             OptBase *o,
                             PbsQueueInterface *p);
    virtual ~PbsConfigDialog();

  public slots:
    void updateGUI();

  protected slots:
    void accept();
    void reject();

  protected:
    OptBase *m_opt;
    PbsQueueInterface *m_pbs;

  private:
    Ui::PbsConfigDialog *ui;

  };
}

/// @endcond
#endif // ENABLE_SSH
#endif // PBSCONFIGDIALOG_H
