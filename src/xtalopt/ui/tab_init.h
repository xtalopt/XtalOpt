/**********************************************************************
  XtalOpt - Tools for advanced crystal optimization

  Copyright (C) 2009-2011 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef TAB_INIT_H
#define TAB_INIT_H

#include <globalsearch/ui/abstracttab.h>

#include "ui_tab_init.h"

namespace GlobalSearch {
class AbstractDialog;
}

namespace XtalOpt {
class RandSpgDialog;
class XtalOpt;

class TabInit : public GlobalSearch::AbstractTab
{
  Q_OBJECT

public:
  explicit TabInit(GlobalSearch::AbstractDialog* parent, XtalOpt* p);
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
    IC_SYMBOL1 = 0,
    IC_SYMBOL2 = 1,
    IC_MINIAD = 2,
  };

  enum MolUnitColumns
  {
    MC_CENTER = 0,
    MC_NUMCENTERS = 1,
    MC_NEIGHBOR = 2,
    MC_NUMNEIGHBORS = 3,
    MC_GEOM = 4,
    MC_DIST = 5
  };

public slots:
  void updateScaledVolume();
  void lockGUI() override;
  void readSettings(const QString& filename = "") override;
  void writeSettings(const QString& filename = "") override;
  void updateGUI() override;
  void getComposition(const QString& str);
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
  void updateMinIAD();
  void addRow();
  void removeRow();
  void removeAll();
  void getCentersAndNeighbors(QList<QString>& centerList, int centerNum,
                              QList<QString>& neighborList, int neighborNum);
  void getNumCenters(int centerNum, int neighborNum,
                     QList<QString>& numCentersList);
  void getNumNeighbors(int centerNum, int neighborNum,
                       QList<QString>& numNeighborsList);
  void getGeom(QList<QString>& geomList, unsigned int numNeighbors);
  void setGeom(unsigned int& geom, QString strGeom);
  void openSpgOptions();

signals:

private:
  Ui::Tab_Init ui;
  RandSpgDialog* m_spgOptions;
};
}

#endif
