/**********************************************************************
  Optimizer - Generic optimizer interface

  Copyright (C) 2010 by David C. Lonie

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
#ifndef OPTIMIZERDIALOG_H
#define OPTIMIZERDIALOG_H

#include <QtGui/QDialog>

class QLineEdit;

namespace GlobalSearch {
  class AbstractDialog;
  class OptBase;
  class Optimizer;

  // Basic input dialog needed for all optimizers
  class OptimizerConfigDialog : public QDialog
  {
    Q_OBJECT;
  public:
    OptimizerConfigDialog(AbstractDialog *parent,
                          OptBase *opt, Optimizer *o);

  public slots:
    void updateState();
    void updateGUI();

  protected:
    OptBase *m_opt;
    Optimizer *m_optimizer;
    QLineEdit *m_lineedit;

  };

} // end namespace GlobalSearch

#endif
/// @endcond
