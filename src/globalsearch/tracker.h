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

#ifndef TRACKER_H
#define TRACKER_H

#include <QList>
#include <QObject>
#include <QReadWriteLock>

namespace GlobalSearch {
class Structure;

/**
 * @class Tracker tracker.h <globalsearch/tracker.h>
 * @brief The Tracker contains a thread-safe list of unique Structures.
 * @author David C. Lonie
 *
 * In simplest terms, the Tracker class is a list of Structures. It
 * provides convenience functions and signals to facilitate access.
 *
 * The Tracker can be used for storage of all Structures generated
 * in a search, or as a FIFO buffer for pending operations by using
 * the append() and popFirst() functions.
 *
 * If you wish to not use the convenience functions, it is possible
 * to access the list of Structures through list() and the mutex
 * through rwLock(), lockForRead(), lockForWrite(), and unlock().
 */
class Tracker : public QObject
{
  Q_OBJECT

public:
  /**
   * Constructor.
   *
   * @param parent The object parent.
   */
  Tracker(QObject* parent = 0);

  /**
   * Destructor.
   */
  virtual ~Tracker() override;

  /**
   * Sets a read lock on the Tracker's mutex.
   */
  void lockForRead() { m_mutex.lockForRead(); };

  /**
   * Sets a write lock on the Tracker's mutex.
   */
  void lockForWrite() { m_mutex.lockForWrite(); };

  /**
   * Unlock the Tracker's mutex.
   */
  void unlock() { m_mutex.unlock(); };

  /**
   * @return The Tracker's read-write mutex
   */
  QReadWriteLock* rwLock() { return &m_mutex; }

  /**
   * @return The Tracker's Structure list
   */
  QList<Structure*>* list() { return &m_list; };

  /**
   * @param i The index of the Structure desired
   * @return A pointer to the Structure at index i
   */
  Structure* at(int i) { return m_list.at(i); };

  /**
   * @param s A list of Structures to append to the Tracker
   *
   * @return true if all Structures were not contained in the list,
   * false if any were already present.
   * @note If the Structure is already present, it will not be added
   * again.
   * @note The Structures are compared using pointer values
   */
  bool append(QList<Structure*> s);

  /**
   * @param s A Structures to append to the Tracker
   *
   * @return true if the Structure was not contained in the list,
   * false if it were already present.
   *
   * @note If the Structure is already present, it will not be added
   * again.
   *
   * @note The Structures are compared using pointer values
   */
  bool append(Structure* s);

  /**
   * Remove and return the first Structure in the Tracker's
   * list. Useful for creating a FIFO buffer.
   *
   * @param s Becomes the Structure at index 0 of the Tracker's
   * list.
   *
   * @return True if operation was successful, false if not
   * (i.e. the list is empty).
   */
  bool popFirst(Structure*& s);

  /**
   * Remove a Structure in the Tracker's list.
   *
   * @param s The Structure to remove.
   *
   * @return True if operation was successful, false if not
   * (i.e. Structure was not in list).
   * @note This does not delete the Structure from memory.
   */
  bool remove(Structure* s);

  /**
   * Test if a Structure is in the Tracker's list.
   *
   * @param s The Structure to check.
   *
   * @return True if operation was successful, false if not
   * (i.e. Structure was not in list).
   */
  bool contains(Structure* s);

  /**
   * @return The number of Structures in the Tracker's list.
   */
  int size();

  /**
   * Remove all Structures from the list.
   * @note This does not delete the Structures from memory.
   */
  void reset();

  /**
   * Remove and delete from memory all Structures from the list.
   */
  void deleteAllStructures();

signals:
  /**
   * Signal emitted when a new Structure is added to the Tracker.
   * @param s A Pointer to the new Structure.
   */
  void newStructureAdded(GlobalSearch::Structure* s);

  /**
   * Signal emitted when then number of Structures in the Tracker
   * changes.
   * @param c The number of new Structure in the Tracker.
   */
  void structureCountChanged(int c);

private:
  QReadWriteLock m_mutex;
  QList<Structure*> m_list;
};
} // end namespace GlobalSearch

#endif
