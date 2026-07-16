/**********************************************************************
  Tracker - A duplicate-checking structure FIFO with its own lock

  Copyright (C) 2010-2011 by David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <search/structure.h>
#include <search/tracker.h>

#include <QList>
#include <QMetaObject>
#include <QReadWriteLock>
#include <QThread>

using namespace std;

namespace Search {

Tracker::Tracker(QObject* parent)
  : QObject(parent), m_mutex(QReadWriteLock::Recursive)
{
}

Tracker::~Tracker()
{
}

bool Tracker::append(QList<Structure*> s)
{
  bool ret = true;
  for (auto* item : s) {
    if (!append(item))
      ret = false;
  }
  return ret;
}

bool Tracker::append(Structure* s)
{
  if (!s)
    return false;

  if (m_members.contains(s)) {
    return false;
  }
  m_list.append(s);
  m_members.insert(s);
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
  m_members.remove(s);
  emit structureCountChanged(m_list.size());
  return true;
}

bool Tracker::remove(Structure* s)
{
  if (m_members.remove(s)) {
    m_list.removeAll(s);
    emit structureCountChanged(m_list.size());
    return true;
  }
  return false;
}

bool Tracker::contains(Structure* s)
{
  return m_members.contains(s);
}

int Tracker::size() const
{
  return m_list.size();
}

void Tracker::reset()
{
  m_list.clear();
  m_members.clear();
  emit structureCountChanged(m_list.size());
}

void Tracker::deleteAllStructures()
{
  QList<Structure*> structures;
  {
    QWriteLocker locker(&m_mutex);
    structures = m_list;
    m_list.clear();
    m_members.clear();
  }

  emit structureCountChanged(0);

  for (auto* s : structures)
    deleteStructure(s);
}

void Tracker::deleteStructure(Structure* s)
{
  if (!s)
    return;

  QThread* ownerThread = s->thread();
  QThread* currentThread = QThread::currentThread();
  if (!ownerThread || ownerThread == currentThread || !ownerThread->isRunning()) {
    delete s;
    return;
  }

  QMetaObject::invokeMethod(s, "deleteLater", Qt::QueuedConnection);
}

} // end namespace Search
