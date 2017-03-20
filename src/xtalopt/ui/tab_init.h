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

#ifndef TAB_INIT_H
#define TAB_INIT_H

#include <globalsearch/ui/abstracttab.h>

#include "ui_tab_init.h"

namespace XtalOpt {
  class RandSpgDialog;
  class XtalOptDialog;
  class XtalOpt;

  class TabInit : public GlobalSearch::AbstractTab
  {
    Q_OBJECT

  public:
    explicit TabInit( XtalOptDialog *parent, XtalOpt *p );
    virtual ~TabInit() override;

    enum CompositionColumns
    {
      CC_SYMBOL = 0,
      CC_ATOMICNUM,
      CC_QUANTITY,
      CC_MASS,
      CC_MINRADIUS
    };

    enum IADColumns
    {
     IC_CENTER = 0,
     IC_NUMCENTERS = 1,
     IC_NEIGHBOR = 2,
     IC_NUMNEIGHBORS = 3,
     IC_GEOM = 4,
     IC_DIST = 5
    };

  public slots:
    void lockGUI() override;
    void readSettings(const QString &filename = "") override;
    void writeSettings(const QString &filename = "") override;
    void updateGUI() override;
    void getComposition(const QString & str);
    void updateComposition();
    void updateCompositionTable();
    void updateDimensions();
    void updateMinRadii();
    void updateFormulaUnits();
    void updateFormulaUnitsListUI();
    void updateInitOptions();
    void adjustVolumesToBePerFU(uint FU);
    void updateNumDivisions();
    void updateA();
    void updateB();
    void updateC();
    void writeA();
    void writeB();
    void writeC();
    void updateIAD();
    void addRow();
    void removeRow();
    void removeAll();
    void getCentersAndNeighbors(QList<QString> & centerList, int centerNum,
            QList<QString> & neighborList, int neighborNum);
    void getNumCenters(int centerNum, int neighborNum, QList<QString> & numCentersList);
    void getNumNeighbors(int centerNum, int neighborNum, QList<QString> & numNeighborsList);
    void getGeom(QList<QString> & geomList, unsigned int numNeighbors);
    void setGeom(unsigned int & geom, QString strGeom);
    void openSpgOptions();

  signals:

  private:
    Ui::Tab_Init ui;
    RandSpgDialog* m_spgOptions;
  };
}

#endif
