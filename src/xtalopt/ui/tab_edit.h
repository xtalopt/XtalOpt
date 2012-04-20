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

#ifndef TAB_EDIT_H
#define TAB_EDIT_H

#include <globalsearch/ui/defaultedittab.h>

class QListWidgetItem;

namespace XtalOpt {
  class XtalOpt;
  class XtalOptDialog;

  class TabEdit : public GlobalSearch::DefaultEditTab
  {
    Q_OBJECT;

  public:
    explicit TabEdit( XtalOptDialog *parent, XtalOpt *p );
    virtual ~TabEdit();

  public slots:
    virtual void updateGUI();
    void readSettings(const QString &filename = "");
    void writeSettings(const QString &filename = "");
    void updateEditWidget();
    void appendOptStep();
    void removeCurrentOptStep();

    void setPreoptimization(bool b);
    void showPreoptimizationConfigDialog();

  protected slots:
    // Returns false if user cancels
    bool generateVASP_POTCAR_info();
    void changePOTCAR(QListWidgetItem *item);
  };
}

#endif
