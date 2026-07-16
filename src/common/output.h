/**********************************************************************
  output - Common output handler interface

  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef COMMON_OUTPUT_H
#define COMMON_OUTPUT_H

#include <common/compatibility/qt_compat.h>

#include <QHash>
#include <QList>
#include <QMutex>
#include <QPair>
#include <QString>
#include <QTextStream>

#include <atomic>
#include <cstdio>
#include <functional>

namespace Common {

// All user output goes through these functions. They write to stdout and any
//   installed output functions, keeping CLI, GUI, tests, and log files consistent.
// Do not use qDebug/cout/cerr/printf for user output.
enum class OutputLevel
{
  /// Normal user-facing message.
  Message,
  /// Recoverable warning that should be visible by default.
  Warning,
  /// User-facing error.
  Error,
  /// Diagnostic/debug message.
  Debug
};

// Buffered entries for messages which will be printed later.
typedef QList<QPair<OutputLevel, QString> > OutputLog;

// Function installed to receive each output entry.
typedef std::function<void(OutputLevel, const QString&)> OutputHandler;

namespace detail {

inline QMutex& outputHandlerMutex()
{
  static QMutex mutex;
  return mutex;
}

inline QHash<int, OutputHandler>& outputHandlers()
{
  static QHash<int, OutputHandler> handlers;
  return handlers;
}

inline int& nextOutputHandlerId()
{
  static int nextId = 1;
  return nextId;
}

// Debug output is off by default; it can be set to true by user.
inline std::atomic<bool>& debugEnabled()
{
  static std::atomic<bool> enabled(false);
  return enabled;
}

} // namespace detail

// Enable/disable Debug-level output (terminal and installed output handlers).
inline void setDebugOutputEnabled(bool on)
{
  detail::debugEnabled().store(on);
}

// Whether Debug-level output is currently enabled.
inline bool debugOutputEnabled()
{
  return detail::debugEnabled().load();
}

inline QString outputPrefix(OutputLevel level)
{
  switch (level) {
    case OutputLevel::Warning:
      return QStringLiteral("Warning: ");
    case OutputLevel::Error:
      return QStringLiteral("Error: ");
    case OutputLevel::Debug:
      return QStringLiteral("Debug: ");
    case OutputLevel::Message:
      return QString();
  }

  return QString();
}

inline QString formatOutput(OutputLevel level, const QString& text)
{
  return outputPrefix(level) + text;
}

// Write one output line to a QTextStream and flush (GUI output handler treats this differently as it stores strings).
inline void writeFormattedOutput(QTextStream& stream, OutputLevel level, const QString& text)
{
  stream << formatOutput(level, text);
  if (!text.endsWith(QLatin1Char('\n')))
    stream << '\n';
  stream.flush();
}

// Register an output handler; returns the id needed to remove it.
inline int addOutputHandler(const OutputHandler& handler)
{
  QtCompat::MutexLocker locker(&detail::outputHandlerMutex());
  const int id = detail::nextOutputHandlerId()++;
  detail::outputHandlers().insert(id, handler);
  return id;
}

inline void removeOutputHandler(int id)
{
  QtCompat::MutexLocker locker(&detail::outputHandlerMutex());
  detail::outputHandlers().remove(id);
}

namespace detail {

// Let only one thread write at a time, so messages do not mix.
inline QMutex& terminalMutex()
{
  static QMutex mutex;
  return mutex;
}

} // namespace detail

inline void writeOutputToTerminal(OutputLevel level, const QString& text)
{
  // Keep one stream for stdout, and flush every line.
  static QTextStream stream(stdout);
  QtCompat::MutexLocker locker(&detail::terminalMutex());
  writeFormattedOutput(stream, level, text);
}

// Emit one entry to the terminal and all installed handlers.
inline void output(OutputLevel level, const QString& text)
{
  // Skip Debug-level output unless it has been turned on.
  if (level == OutputLevel::Debug && !detail::debugEnabled().load())
    return;

  // Always write to stdout (Terminal isn't handled as a handler!)
  writeOutputToTerminal(level, text);

  QList<OutputHandler> activeOutputHandlers;
  {
    QtCompat::MutexLocker locker(&detail::outputHandlerMutex());
    activeOutputHandlers = detail::outputHandlers().values();
  }

  for (const auto& handler : activeOutputHandlers)
    handler(level, text);
}

inline void output(const QString& text)
{
  output(OutputLevel::Message, text);
}

// Emit every entry in a buffered log.
inline void output(const OutputLog& log)
{
  for (int i = 0; i < log.size(); ++i)
    output(log.at(i).first, log.at(i).second);
}

// Append one entry to log, if non-null.
inline void appendOutput(OutputLog* log, const QString& text,
                         OutputLevel level = OutputLevel::Message)
{
  if (log)
    log->append(qMakePair(level, text));
}

inline void message(const QString& text)
{
  output(OutputLevel::Message, text);
}

inline void warning(const QString& text)
{
  output(OutputLevel::Warning, text);
}

inline void error(const QString& text)
{
  output(OutputLevel::Error, text);
}

inline void debug(const QString& text)
{
  output(OutputLevel::Debug, text);
}

} // namespace Common

#endif // COMMON_OUTPUT_H
