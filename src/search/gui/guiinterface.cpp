/**********************************************************************
  guiinterface - Install output and prompt handlers for GUI

  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <search/gui/guiinterface.h>

#include <search/gui/abstractdialog.h>
#include <common/output.h>
#include <search/queuemanager.h>
#include <search/search.h>

#include <QMetaObject>
#include <QMetaType>
#include <QPointer>
#include <QThread>
#include <QVector>

namespace Search {

// Connect the SearchBase prompt handlers and signals to AbstractDialog.
// Prompt functions for the GUI. Do not call them while holding a shared lock.
void installGuiInterface(AbstractDialog& dialog, SearchBase& search)
{
  qRegisterMetaType<QVector<int> >("QVector<int>");

  search.setDecisionPromptHandler(
    [&dialog](const QString& message, bool defaultValue) -> bool {
      bool accepted = defaultValue;
      QMetaObject::invokeMethod(&dialog, "showBooleanPromptDialogOnGuiThread",
                                QThread::currentThread() == dialog.thread() ? Qt::DirectConnection
                                  : Qt::BlockingQueuedConnection, Q_ARG(QString, message),
                                Q_ARG(bool*, &accepted));
      return accepted;
    });
  search.setPasswordPromptHandler(
    [&dialog](const QString& message, QString& password) -> bool {
      bool accepted = false;
      QMetaObject::invokeMethod(&dialog, "showPasswordPromptDialogOnGuiThread",
                                QThread::currentThread() == dialog.thread() ? Qt::DirectConnection
                                  : Qt::BlockingQueuedConnection, Q_ARG(QString, message),
                                Q_ARG(QString*, &password), Q_ARG(bool*, &accepted));
      return accepted;
    });

  QObject::connect(&search, &SearchBase::sessionStarted, &dialog, &AbstractDialog::updateGUI);
  QObject::connect(&search, &SearchBase::sessionStarted, &dialog, &AbstractDialog::lockGUI);
  QObject::connect(&search, &SearchBase::progressRangeChanged,
                   &dialog, &AbstractDialog::handleProgressStarted);
  QObject::connect(&search, &SearchBase::progressValueChanged,
                   &dialog, &AbstractDialog::applyProgressUpdate);
  QObject::connect(&search, &SearchBase::progressEnded,
                   &dialog, &AbstractDialog::handleProgressEnded);
  QObject::connect(&search, &SearchBase::errorDialogRequested,
                   &dialog,
                   [&dialog](const QString& message) {
                     QMetaObject::invokeMethod(&dialog, "showErrorDialogOnGuiThread",
                                               Qt::DirectConnection, Q_ARG(QString, message));
                   });
  QObject::connect(search.queue(), &QueueManager::newStatusOverview,
                   &dialog, &AbstractDialog::updateStatus);

  // Send Common output to the GUI Log tab. QPointer protects queued output
  //   after the dialog is destroyed.
  QPointer<AbstractDialog> dialogPtr(&dialog);
  const int outHandlerId = Common::addOutputHandler(
    [dialogPtr](Common::OutputLevel level, const QString& text) {
      if (!dialogPtr)
        return;
      QMetaObject::invokeMethod(dialogPtr.data(), "newLog", Qt::QueuedConnection,
                                Q_ARG(QString, Common::formatOutput(level, text)));
    });
  QObject::connect(&dialog, &QObject::destroyed, [outHandlerId]() { Common::removeOutputHandler(outHandlerId); });
  QObject::connect(&search, &QObject::destroyed, [outHandlerId]() { Common::removeOutputHandler(outHandlerId); });
}

} // namespace Search
