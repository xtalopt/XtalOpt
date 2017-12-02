/**********************************************************************
  SlottedWaitCondition - Simple wrapper around QWaitCondition with
  wake slots.

  Copyright (C) 2010-2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef SLOTTEDWAITCONDITION_H
#define SLOTTEDWAITCONDITION_H

#include <QMutex>
#include <QObject>
#include <QWaitCondition>

namespace GlobalSearch {

/**
 * @class SlottedWaitCondition slottedwaitcondition.h
<globalsearch/slottedwaitcondition.h>
 *
 * @brief A wrapper for QWaitCondition that has slots for wakeOne
 * and wakeAll. See warning in expanded documentation.
 *
 * @warning Misuse of this class can easily lead to a deadlock. If
 * all calls to the wake* slots are made before wait() is called,
 * the code may hang indefinitely. Ensure that wait() is not called
 * without setting a timeout to prevent this. For similar reasons,
 * it is recommended that this construct is only used for trivial
 * applications, such as GUI updates.
 *
 * This class adds a few new functions to QWaitCondition:
 *   - wakeOneSlot(): A slot that calls QWaitCondition::wakeOne()
 *   - wakeAllSlot(): A slot that calls QWaitCondition::wakeAll()
 *   - mutex(): Returns a pointer to the internal mutex (see below)
 *   - prewaitLock(): A slot that locks the internal mutex (see below)
 *   - postwaitUnlock(): A slot that unlocks the internal mutex (see below)
 *
 * SlottedWaitCondition also includes an internal mutex that can be
 * used for create critical sections before and after calls to
 * wait(). Normally an external mutex is used for this, but calls to
 * wait(), prewaitLock(), and postwaitUnlock() use the internal
 * mutex. Calling mutex() will return a pointer to this
 * mutex. Typical usage of this wait condition is:
@code
SlottedWaitCondition slottedWC;
...
connect(sender, SIGNAL(somethingThatFreesSlottedWCsResources()),
      &slottedWC, SLOT(wakeAllSlot()));

slottedWC->prewaitLock();
// Do any work that needs to be peformed in a critical section before waiting
...

// Check every 250 milliseconds that we still need to be waiting
while (someConditionIsTrue) {
slottedWC->wait(250);
}

// Do any work that needs to be done in a critical section after waking
...

slottedWC->postwaitUnlock();
@endcode
 *
 * @author David C. Lonie
 */
class SlottedWaitCondition : public QObject, public QWaitCondition
{
  Q_OBJECT
private:
  /**
   * Convenience mutex used internally. See class documentation for
   * more information.
   * @sa mutex
   * @sa prewaitLock
   * @sa postwaitUnlock
   * @sa wait
   */
  QMutex m_mutex;

public:
  /**
   * Constructor.
   *
   * @param parent Parent for QObject.
   */
  SlottedWaitCondition(QObject* parent);

  /**
   * Destructor.
   */
  virtual ~SlottedWaitCondition() override;

  /**
   * @return A pointer to the internal mutex, m_mutex.
   * @sa prewaitLock
   * @sa postwaitUnlock
   * @sa wait
   */
  QMutex* mutex() { return &m_mutex; }

  /**
   * Wait until woken up by one of:
   *   - QWaitCondition::wakeOne()
   *   - QWaitCondition::wakeAll()
   *   - SlottedWaitCondition::wakeOneSlot()
   *   - SlottedWaitCondition::wakeAllSlot()
   *
   * @warning Misuse of this function can easily lead to a
   * deadlock. If all calls to the wake* slots are made before
   * wait() is called, the code may hang indefinitely. Ensure that
   * wait() is only called using a reasonable timeout to prevent
   * this. For similar reasons, it is recommended that this
   * construct is only used for trivial applications, such as GUI
   * updates.
   *
   * @param timeout Timeout in milliseconds. After this time, the
   * wait condition will wake itself and continue.
   *
   * This function expects the internal mutex, m_mutex, to be locked
   * before prior to using being called. This can be accomplished by
   * calling lock() on mutex() or prewaitLock(). The internal mutex
   * will be locked before returning from this function. The mutex
   * can be unlocked by calling the convenience function
   * postwaitUnlock(). See class description for example usage.
   */
  void wait(unsigned long timeout) { QWaitCondition::wait(&m_mutex, timeout); }

public slots:
  /**
   * Slot that calls QWaitCondition::wakeOne()
   */
  void wakeOneSlot() { QWaitCondition::wakeOne(); }

  /**
   * Slot that calls QWaitCondition::wakeAll()
   */
  void wakeAllSlot() { QWaitCondition::wakeAll(); }

  /**
   * Lock the internal mutex, m_mutex, to create a critical section
   * before calling wait().
   */
  void prewaitLock() { m_mutex.lock(); }

  /**
   * Unlock the internal mutex, m_mutex, to end the critical section
   * after calling wait().
   */
  void postwaitUnlock() { m_mutex.unlock(); }
};
}

#endif // SLOTTEDWAITCONDITION
