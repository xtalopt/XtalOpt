/**********************************************************************
  ExampleSearch -- A tool for analysing a matrix-substrate docking problem

  Copyright (C) 2012 by David C. Lonie

  This library is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as
  published by the Free Software Foundation; either version 2.1 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public icense for more details.
 ***********************************************************************/

#ifndef TAB_EDIT_H
#define TAB_EDIT_H

#include <globalsearch/ui/defaultedittab.h>

namespace ExampleSearch {
  class ExampleSearch;
  class ExampleSearchDialog;

  class TabEdit : public GlobalSearch::DefaultEditTab
  {
    Q_OBJECT

  public:
    explicit TabEdit( ExampleSearchDialog *parent, ExampleSearch *p );
    virtual ~TabEdit();

  public slots:
    void readSettings(const QString &filename = "");
    void writeSettings(const QString &filename = "");
  };
}

#endif
