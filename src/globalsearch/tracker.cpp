/**********************************************************************
  Tracker - A thread safe duplicate checking structure FIFO

  Copyright (C) 2010-2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <globalsearch/structure.h>
#include <globalsearch/tracker.h>

#include <QDebug>
#include <QList>
#include <QReadWriteLock>

using namespace Eigen;
using namespace std;

namespace GlobalSearch {

Tracker::Tracker(QObject* parent)
  : QObject(parent), m_mutex(QReadWriteLock::Recursive)
{
}

Tracker::~Tracker()
{
  lockForWrite();
}

bool Tracker::append(QList<Structure*> s)
{
  bool ret = true;
  for (int i = 0; i < s.size(); i++) {
    if (!append(s.at(i)))
      ret = false;
  }
  return ret;
}

bool Tracker::append(Structure* s)
{
  if (m_list.contains(s)) {
    return false;
  }
  m_list.append(s);
  emit newStructureAdded(s);
  emit structureCountChanged(m_list.size());
  return true;
}

bool Tracker::popFirst(Structure*& s)
{
  if (m_list.isEmpty()) {
    return false;
  }
  s = m_list.takeFirst();
  emit structureCountChanged(m_list.size());
  return true;
}

bool Tracker::remove(Structure* s)
{
  if (m_list.removeAll(s)) { // returns number of entries removed
    emit structureCountChanged(m_list.size());
    return true;
  }
  return false;
}

bool Tracker::contains(Structure* s)
{
  bool b = m_list.contains(s);
  return b;
}

int Tracker::size()
{
  return m_list.size();
}

void Tracker::reset()
{
  m_list.clear();
  emit structureCountChanged(m_list.size());
}

void Tracker::deleteAllStructures()
{
  Structure* s = 0;
  for (int i = 0; i < m_list.size(); i++) {
    s = m_list.at(i);
    s->lock().lockForWrite();
    s->deleteLater();
  }
  m_list.clear();
  emit structureCountChanged(m_list.size());
}

} // end namespace GlobalSearch
