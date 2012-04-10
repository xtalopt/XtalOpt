/**********************************************************************
  ExampleSearch

  Copyright (C) 2012 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifndef EXAMPLESEARCH_H
#define EXAMPLESEARCH_H

#include <globalsearch/optbase.h>

#include <Eigen/Geometry>

#include <QtCore/QDebug>
#include <QtCore/QStringList>
#include <QtCore/QReadWriteLock>

#include <QtGui/QInputDialog>

namespace GlobalSearch {
  class SlottedWaitCondition;
  class Structure;
}

namespace ExampleSearch {
  class ExampleSearchDialog;

  class ExampleSearch : public GlobalSearch::OptBase
  {
    Q_OBJECT

   public:
    explicit ExampleSearch(ExampleSearchDialog *parent);
    virtual ~ExampleSearch();

    enum OptTypes {
      OT_GAMESS = 0,
      OT_MOPAC
    };

    enum QueueInterfaces {
      QI_LOCAL = 0
#ifdef ENABLE_SSH
      ,
      QI_PBS,
      QI_SGE,
      QI_LSF,
      QI_SLURM,
      QI_LOADLEVELER
#endif // ENABLE_SSH
    };

    GlobalSearch::Structure* replaceWithRandom(GlobalSearch::Structure *s,
                                               const QString & reason = "");

    GlobalSearch::Structure *generateRandomStructure();

    bool checkLimits();

  public slots:
    void startSearch();
    void generateNewStructure();
    void initializeAndAddStructure(GlobalSearch::Structure *structure);

  private:
    GlobalSearch::SlottedWaitCondition *m_initWC;
    QMutex *m_structureInitMutex;
  };

} // end namespace ExampleSearch

#endif
