/**********************************************************************
  Tracker - A thread safe duplicate checking structure FIFO

  Copyright (C) 2010 by David C. Lonie

  This file is part of the Avogadro molecular editor project.
  For more information, see <http://avogadro.openmolecules.net/>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifndef TRACKER_H
#define TRACKER_H

#include <globalsearch/structure.h>

#include <QDebug>
#include <QList>
#include <QReadWriteLock>

namespace GlobalSearch {

  class Tracker : public QObject
  {
    Q_OBJECT
  private:
    QReadWriteLock m_mutex;
    QList<Structure*> m_list;
  public:
    Tracker(QObject *parent = 0);
    virtual ~Tracker();

    void lockForRead() {m_mutex.lockForRead();};
    void lockForWrite() {m_mutex.lockForWrite();};
    void unlock() {m_mutex.unlock();};
    QReadWriteLock* rwLock() {return &m_mutex;}
    QList<Structure*>* list() {return &m_list;};
    Structure* at(int i) {return m_list.at(i);};
    bool append(QList<Structure*> s);
    bool append(Structure* s);
    bool appendAndUnlock(Structure* s);
    bool popFirst(Structure *&s);
    bool remove(Structure *s);
    bool contains(Structure* s);
    int size();
    void reset();
    void deleteAllStructures();

  signals:
    void newStructureAdded(Structure*);
    void structureCountChanged(int);
  };
} // end namespace Avogadro

#endif
