/**********************************************************************
  qt_compat - Compatibility shims for Qt API changes across versions.

  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef COMMON_QT_COMPAT_H
#define COMMON_QT_COMPAT_H

#include <QtGlobal>
#include <QList>
#include <QMutex>
#include <QMutexLocker>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QTextStream>

// Qt compatibility helpers for Qt 5.4 through Qt 6.
namespace QtCompat {

// QString split behavior changed in Qt 5.14.
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
static const QString::SplitBehavior SkipEmptyParts = QString::SkipEmptyParts;
static const QString::SplitBehavior KeepEmptyParts = QString::KeepEmptyParts;
#else
static const Qt::SplitBehaviorFlags SkipEmptyParts = Qt::SkipEmptyParts;
static const Qt::SplitBehaviorFlags KeepEmptyParts = Qt::KeepEmptyParts;
#endif

// Qt stream end line changed in Qt 5.14.
inline QTextStream& endl(QTextStream& ts)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
  return ::endl(ts);
#else
  return Qt::endl(ts);
#endif
}

// Qt mutex locker changed in Qt 6.
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
typedef QMutexLocker<QMutex> MutexLocker;
#else
typedef QMutexLocker MutexLocker;
#endif

// QList item swap changed between Qt 5.13 and Qt 6.
template<typename T>
inline void listSwapItemsAt(QList<T>& list, int i, int j)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
  list.swap(i, j);
#else
  list.swapItemsAt(i, j);
#endif
}

// QProcess error signal changed between Qt 5.6 and Qt 6.
template<typename Receiver, typename Slot>
inline void connectProcessError(QProcess* proc, Receiver* receiver, Slot slot)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
  QObject::connect(proc, &QProcess::errorOccurred, receiver, slot);
#else
  QObject::connect(proc, static_cast<void(QProcess::*)(QProcess::ProcessError)>(&QProcess::error),
    receiver, slot);
#endif
}

// Start a process from one command string. Windows uses cmd.exe for its
// command and batch-file support. Qt 5.15 changed the other calls.
inline void processStartCommand(QProcess& proc, const QString& command)
{
#if defined(Q_OS_WIN)
  proc.setNativeArguments("/S /C \"" + command + "\"");
  proc.start("cmd.exe", QStringList());
#elif QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  proc.startCommand(command);
#elif QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
  // Split the command first.
  const QStringList parts = QProcess::splitCommand(command);
  proc.start(parts.value(0), parts.mid(1));
#else
  proc.start(command);
#endif
}

} // namespace QtCompat

#endif // COMMON_QT_COMPAT_H
