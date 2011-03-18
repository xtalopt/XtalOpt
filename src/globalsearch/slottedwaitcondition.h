/**********************************************************************
  SlottedWaitCondition - Simple wrapper around QWaitCondition with
  wake slots.

  Copyright (C) 2010 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifndef SLOTTEDWAITCONDITION_H
#define SLOTTEDWAITCONDITION_H

#include <QtCore/QObject>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>

namespace GlobalSearch {

  class SlottedWaitCondition : public QObject, public QWaitCondition
  {
    Q_OBJECT;
  private:
    QMutex m_mutex; // Convenience mutex for this waitcondition

  public:
    SlottedWaitCondition(QObject *parent);
    virtual ~SlottedWaitCondition();
    QMutex* mutex() {return &m_mutex;}

    void wait(unsigned long timeout = ULONG_MAX) {
      QWaitCondition::wait(&m_mutex, timeout);}

  public slots:
    void wakeOneSlot() {QWaitCondition::wakeOne();}
    void wakeAllSlot() {QWaitCondition::wakeAll();}
    void prewaitLock() {m_mutex.lock();}
    void postwaitUnlock() {m_mutex.unlock();}
  };

}

#endif // SLOTTEDWAITCONDITION
