/**********************************************************************
  LsfConfigDialog -- Setup for remote LSF queues

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

#ifndef LSFCONFIGDIALOG_H
#define LSFCONFIGDIALOG_H

#ifdef ENABLE_SSH

//Doxygen should ignore this file:
/// @cond

#include <QtGui/QDialog>

namespace Ui {
  class LsfConfigDialog;
}

namespace GlobalSearch {
  class AbstractDialog;
  class OptBase;
  class LsfQueueInterface;

  class LsfConfigDialog : public QDialog
  {
    Q_OBJECT

  public:

    explicit LsfConfigDialog(AbstractDialog *parent,
                             OptBase *o,
                             LsfQueueInterface *p);
    virtual ~LsfConfigDialog();

  public slots:
    void updateGUI();

  protected slots:
    void accept();
    void reject();

  protected:
    OptBase *m_opt;
    LsfQueueInterface *m_lsf;

  private:
    Ui::LsfConfigDialog *ui;

  };
}

/// @endcond
#endif // ENABLE_SSH
#endif // LSFCONFIGDIALOG_H
