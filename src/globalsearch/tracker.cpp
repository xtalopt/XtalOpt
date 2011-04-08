/**********************************************************************
  Tracker - A thread safe duplicate checking structure FIFO

  Copyright (C) 2010-2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include <globalsearch/tracker.h>
#include <globalsearch/structure.h>

#include <QtCore/QList>
#include <QtCore/QDebug>
#include <QtCore/QReadWriteLock>

using namespace Avogadro;
using namespace OpenBabel;
using namespace Eigen;
using namespace std;

namespace GlobalSearch {

  Tracker::Tracker(QObject *parent) :
    QObject(parent)
  {
  }

  Tracker::~Tracker()
  {
    lockForWrite();
  }

  bool Tracker::append(QList<Structure*> s) {
    bool ret = true;
    for (int i = 0; i < s.size(); i++) {
      if (!append(s.at(i)))
        ret = false;
    }
    return ret;
  }

  bool Tracker::append(Structure* s) {
    if (m_list.contains(s)) {
      return false;
    }
    m_list.append(s);
    emit newStructureAdded(s);
    emit structureCountChanged(m_list.size());
    return true;
  }

  bool Tracker::popFirst(Structure *&s) {
    if (m_list.isEmpty()) {
      return false;
    }
    s = m_list.takeFirst();
    emit structureCountChanged(m_list.size());
    return true;
  }

  bool Tracker::remove(Structure *s) {
    if (m_list.removeAll(s)) { // returns number of entries removed
      emit structureCountChanged(m_list.size());
      return true;
    }
    return false;
  }

  bool Tracker::contains(Structure* s) {
    bool b = m_list.contains(s);
    return b;
  }

  int Tracker::size() {
    return m_list.size();
  }

  void Tracker::reset() {
    m_list.clear();
    emit structureCountChanged(m_list.size());
  }

  void Tracker::deleteAllStructures() {
    Structure *s = 0;
    for (int i = 0; i < m_list.size(); i++) {
      s = m_list.at(i);
      s->lock()->lockForWrite();
      s->deleteLater();
    }
    m_list.clear();
    emit structureCountChanged(m_list.size());
  }

} // end namespace GlobalSearch
