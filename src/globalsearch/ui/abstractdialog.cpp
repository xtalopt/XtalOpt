/**********************************************************************
  AbstractDialog -- A base dialog class for use with libglobalsearch
  projects. See the accompanying .ui file for a Qt Designer template.

  Copyright (C) 2010-2011 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <globalsearch/ui/abstractdialog.h>

#include <globalsearch/optbase.h>
#include <globalsearch/optimizer.h>
#include <globalsearch/queuemanager.h>
#include <globalsearch/structure.h>
#include <globalsearch/tracker.h>

#include <QApplication>
#include <QFileDialog>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>

#include <QSettings>
#include <QTabWidget>
#include <QTimer>
#include <QtConcurrent>

using namespace std;

namespace GlobalSearch {

AbstractDialog::AbstractDialog(QWidget* parent, Qt::WindowFlags f)
  : QDialog(parent, f), m_opt(0), m_ownsOptBase(false)
{
  // Initialize vars, connections, etc
  progMutex = new QMutex;
  progTimer = new QTimer;

  setWindowFlags(Qt::Window);
}

void AbstractDialog::initialize()
{
  // Connections
  connect(this, SIGNAL(tabsReadSettings(const QString&)), this,
          SLOT(reemitTabsReadSettings(const QString&)), Qt::DirectConnection);
  // Leave this as an autoconnection to prevent deadlocks on shutdown
  connect(this, SIGNAL(tabsWriteSettings(const QString&)), this,
          SLOT(reemitTabsWriteSettings(const QString&)));

  connect(ui_push_begin, SIGNAL(clicked()), this, SLOT(startSearch()));
  connect(ui_push_save, SIGNAL(clicked()), this, SLOT(saveSession()));
  connect(ui_push_resume, SIGNAL(clicked()), this, SLOT(resumeSession()));

  connect(m_opt, SIGNAL(sessionStarted()), this, SLOT(updateGUI()));
  connect(m_opt, SIGNAL(sessionStarted()), this, SLOT(lockGUI()));
  connect(m_opt, SIGNAL(readOnlySessionStarted()), this, SLOT(updateGUI()));
  connect(m_opt, SIGNAL(readOnlySessionStarted()), this, SLOT(lockGUI()));
  connect(m_opt->queue(), SIGNAL(newStatusOverview(int, int, int)), this,
          SLOT(updateStatus(int, int, int)));
  connect(this, SIGNAL(sig_updateStatus(int, int, int)), this,
          SLOT(updateStatus_(int, int, int)));

  connect(progTimer, SIGNAL(timeout()), this, SLOT(repaintProgressBar_()),
          Qt::QueuedConnection);
  connect(this, SIGNAL(sig_startProgressUpdate(const QString&, int, int)), this,
          SLOT(startProgressUpdate_(const QString&, int, int)),
          Qt::QueuedConnection);
  connect(this, SIGNAL(sig_stopProgressUpdate()), this,
          SLOT(stopProgressUpdate_()), Qt::QueuedConnection);
  connect(this, SIGNAL(sig_updateProgressMinimum(int)), this,
          SLOT(updateProgressMinimum_(int)), Qt::QueuedConnection);
  connect(this, SIGNAL(sig_updateProgressMaximum(int)), this,
          SLOT(updateProgressMaximum_(int)), Qt::QueuedConnection);
  connect(this, SIGNAL(sig_updateProgressValue(int)), this,
          SLOT(updateProgressValue_(int)), Qt::QueuedConnection);
  connect(this, SIGNAL(sig_updateProgressLabel(const QString&)), this,
          SLOT(updateProgressLabel_(const QString&)), Qt::QueuedConnection);
  connect(this, SIGNAL(sig_repaintProgressBar()), this,
          SLOT(repaintProgressBar_()), Qt::QueuedConnection);

  connect(m_opt, SIGNAL(warningStatement(const QString&)), this,
          SLOT(newWarning(const QString&)), Qt::QueuedConnection);
  connect(m_opt, SIGNAL(debugStatement(const QString&)), this,
          SLOT(newDebug(const QString&)), Qt::QueuedConnection);
  connect(m_opt, SIGNAL(errorStatement(const QString&)), this,
          SLOT(newError(const QString&)), Qt::QueuedConnection);
  connect(m_opt, SIGNAL(messageStatement(const QString&)), this,
          SLOT(newMessage(const QString&)), Qt::QueuedConnection);
  connect(this, SIGNAL(sig_messageBox(const QString&)), this,
          SLOT(messageBox_(const QString&)), Qt::QueuedConnection);

  // Select the first tab by default
  ui_tabs->setCurrentIndex(0);

  // Hide the progress bar/label
  ui_label_prog->setVisible(false);
  ui_progbar->setVisible(false);
  readSettings();

  // Disable the save button until a session begins
  ui_push_save->setEnabled(false);
}

AbstractDialog::~AbstractDialog()
{
  if (m_ownsOptBase)
    delete m_opt;
}

void AbstractDialog::disconnectGUI()
{
  emit tabsDisconnectGUI();
  disconnect(m_opt, SIGNAL(sessionStarted()), this, SLOT(updateGUI()));
  disconnect(m_opt, SIGNAL(readOnlySessionStarted()), this, SLOT(updateGUI()));
  disconnect(this, SIGNAL(sig_updateStatus(int, int, int)), this,
             SLOT(updateStatus_(int, int, int)));
}

void AbstractDialog::lockGUI()
{
  ui_push_resume->setDisabled(true);
  ui_push_begin->setDisabled(true);
  // This function is called when a session begins. Enable saves:
  ui_push_save->setEnabled(true);
  emit tabsLockGUI();
}

void AbstractDialog::updateGUI()
{
  setWindowTitle(QString("%1 - %2 @ %3%4")
                   .arg(m_opt->getIDString())
                   .arg(m_opt->description)
                   .arg(m_opt->host)
                   .arg(m_opt->readOnly ? " (Read-Only mode)" : ""));
  emit tabsUpdateGUI();
}

void AbstractDialog::resumeSession()
{
  QString filename;
  filename = QFileDialog::getOpenFileName(
    this, QString("Select .state file to resume"), m_opt->filePath,
    "*.state;;*.*", 0, QFileDialog::DontUseNativeDialog);

  if (!filename.isEmpty())
    QtConcurrent::run(this, &AbstractDialog::resumeSession_, filename);
}

void AbstractDialog::resumeSession_(const QString& filename)
{
  startProgressUpdate(tr("Resuming session..."), 0, 0);
  m_opt->tracker()->lockForWrite();
  m_opt->tracker()->deleteAllStructures();
  m_opt->tracker()->unlock();
  if (!m_opt->load(filename)) {
    stopProgressUpdate();
    m_opt->isStarting = false;
    return;
  }
  m_opt->emitStartingSession();

  stopProgressUpdate();

  // Emit session started signals
  if (m_opt->readOnly)
    m_opt->emitReadOnlySessionStarted();
  else
    m_opt->emitSessionStarted();
}

void AbstractDialog::updateStatus_(int opt, int run, int fail)
{
  ui_label_opt->setText(QString::number(opt));
  ui_label_run->setText(QString::number(run));
  ui_label_fail->setText(QString::number(fail));
}

void AbstractDialog::startProgressUpdate_(const QString& text, int min, int max)
{
  // progMutex->lock();
  ui_progbar->reset();
  ui_progbar->setRange(min, max);
  ui_progbar->setValue(min);
  ui_label_prog->setText(text);
  ui_progbar->setVisible(true);
  ui_label_prog->setVisible(true);
  repaintProgressBar();
  progTimer->start(1000);
}

void AbstractDialog::stopProgressUpdate_()
{
  ui_progbar->reset();
  ui_label_prog->setText("");
  ui_progbar->setVisible(false);
  ui_label_prog->setVisible(false);
  progTimer->stop();
  // progMutex->unlock();
  repaintProgressBar();
}

void AbstractDialog::updateProgressMinimum_(int min)
{
  ui_progbar->setMinimum(min);
  repaintProgressBar();
}

void AbstractDialog::updateProgressMaximum_(int max)
{
  ui_progbar->setMaximum(max);
  repaintProgressBar();
}

void AbstractDialog::updateProgressValue_(int val)
{
  ui_progbar->setValue(val);
  repaintProgressBar();
}

void AbstractDialog::updateProgressLabel_(const QString& text)
{
  ui_label_prog->setText(text);
  repaintProgressBar();
}

void AbstractDialog::repaintProgressBar_()
{
  ui_label_prog->repaint();
  ui_progbar->repaint();
}

/// @cond
void AbstractDialog::reemitTabsWriteSettings(const QString& filename)
{
  if (QThread::currentThread() == qApp->thread()) {
    // In GUI thread, direct connection
    emit tabsWriteSettingsDirect(filename);
  } else {
    // In a worker thread, use BlockingQueued
    emit tabsWriteSettingsBlockingQueued(filename);
  }
}

void AbstractDialog::reemitTabsReadSettings(const QString& filename)
{
  if (QThread::currentThread() == qApp->thread()) {
    // In GUI thread, direct connection
    emit tabsReadSettingsDirect(filename);
  } else {
    // In a worker thread, use BlockingQueued
    emit tabsReadSettingsBlockingQueued(filename);
  }
}

void AbstractDialog::errorBox_(const QString& s)
{
  if (m_opt->usingGUI()) {
    QMessageBox::critical(this, "Error", s);
  } else {
    qDebug() << "Error: " << s;
  }
}

void AbstractDialog::messageBox_(const QString& s)
{
  if (m_opt->usingGUI()) {
    QMessageBox::information(this, "Message", s);
  }
}

/// @endcond
}
