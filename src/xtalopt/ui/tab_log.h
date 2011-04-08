/**********************************************************************
  XtalOpt - Tools for advanced crystal optimization

  Copyright (C) 2009-2011 by David Lonie

  This library is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as
  published by the Free Software Foundation; either version 2.1 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public icense for more details.
 ***********************************************************************/

#ifndef TAB_LOG_H
#define TAB_LOG_H

#include <globalsearch/ui/abstracttab.h>

#include "ui_tab_log.h"

namespace XtalOpt {
  class XtalOptDialog;
  class XtalOpt;

  class TabLog : public GlobalSearch::AbstractTab
  {
    Q_OBJECT

  public:
    explicit TabLog( XtalOptDialog *parent, XtalOpt *p );
    virtual ~TabLog();

  public slots:
    void disconnectGUI();
    void newLog(const QString & info);

  signals:

  private:
    Ui::Tab_Log ui;
  };
}

#endif
