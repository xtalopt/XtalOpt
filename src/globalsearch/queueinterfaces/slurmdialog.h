/**********************************************************************
  SlurmConfigDialog -- Setup for remote SLURM queues

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

#ifndef SLURMCONFIGDIALOG_H
#define SLURMCONFIGDIALOG_H

#ifdef ENABLE_SSH

//Doxygen should ignore this file:
/// @cond

#include <QtGui/QDialog>

namespace Ui {
  class SlurmConfigDialog;
}

namespace GlobalSearch {
  class AbstractDialog;
  class OptBase;
  class SlurmQueueInterface;

  class SlurmConfigDialog : public QDialog
  {
    Q_OBJECT

  public:

    explicit SlurmConfigDialog(AbstractDialog *parent,
                             OptBase *o,
                             SlurmQueueInterface *p);
    virtual ~SlurmConfigDialog();

  public slots:
    void updateGUI();

  protected slots:
    void accept();
    void reject();

  protected:
    OptBase *m_opt;
    SlurmQueueInterface *m_slurm;

  private:
    Ui::SlurmConfigDialog *ui;

  };
}

/// @endcond
#endif // ENABLE_SSH
#endif // SLURMCONFIGDIALOG_H
