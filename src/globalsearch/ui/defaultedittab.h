/**********************************************************************
  DefaultEditTab - Simple implementation of AbstractEditTab

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

#ifndef DEFAULTEDITTAB_H
#define DEFAULTEDITTAB_H

#include <globalsearch/ui/abstractedittab.h>

namespace Ui {
  class DefaultEditTab;
}

namespace GlobalSearch {
  class AbstractDialog;
  class OptBase;

  class DefaultEditTab : public GlobalSearch::AbstractEditTab
  {
    Q_OBJECT;

  public:
    explicit DefaultEditTab(AbstractDialog *dialog,
                            OptBase *opt);
    virtual ~DefaultEditTab();

  protected slots:
    virtual void initialize();

  private:
    Ui::DefaultEditTab *ui;
  };
}

#endif
