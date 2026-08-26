/**********************************************************************
  AbstractDialog - A base dialog class for use with libsearch
  projects. See the accompanying .ui file for a Qt Designer template.

  Copyright (C) 2010-2011 by David Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <search/gui/abstractdialog.h>
#include <search/gui/guiinterface.h>
#include <common/gui/qt_compat_gui.h>

#include <search/search.h>
#include <search/optimizer.h>
#include <search/structure.h>
#include <search/tracker.h>

#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QFontMetrics>
#include <QInputDialog>
#include <QWriteLocker>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPointer>
#include <QProgressBar>
#include <QPushButton>
#include <QSizePolicy>

#include <QTabWidget>
#include <QTimer>
#include <QThread>
#include <QtConcurrent>

using namespace std;

namespace Search {
namespace {

QString elidedProgressText(QLabel* label, const QString& text)
{
  if (!label)
    return text;

  const int availableWidth = qMax(label->contentsRect().width(), label->width());
  return label->fontMetrics().elidedText(text, Qt::ElideMiddle, qMax(availableWidth, 120));
}

void setProgressLabelText(QLabel* label, const QString& text)
{
  if (!label)
    return;

  label->setToolTip(text);
  label->setText(elidedProgressText(label, text));
}

} // namespace

AbstractDialog::AbstractDialog(QWidget* parent, Qt::WindowFlags f)
  : QDialog(parent, f), m_search(0), m_ownsSearchBase(false)
{
  // Initialize vars, connections, etc
  progTimer = new QTimer;

  setWindowFlags(Qt::Window);
}

void AbstractDialog::initialize()
{
  if (m_search)
    installGuiInterface(*this, *m_search);

  // Connections
  connect(ui_push_begin,  &QPushButton::clicked, this, &AbstractDialog::startSearch);
  connect(ui_push_save,   &QPushButton::clicked, this, &AbstractDialog::saveSession);
  connect(ui_push_resume, &QPushButton::clicked, this, &AbstractDialog::resumeSession);
  connect(this, &AbstractDialog::sig_updateStatus, this, &AbstractDialog::updateStatus_);

  connect(progTimer, &QTimer::timeout,
          this, &AbstractDialog::repaintProgressBar_, Qt::QueuedConnection);
  connect(this, &AbstractDialog::sig_startProgressUpdate,
          this, &AbstractDialog::startProgressUpdate_, Qt::QueuedConnection);
  connect(this, &AbstractDialog::sig_stopProgressUpdate,
          this, &AbstractDialog::stopProgressUpdate_, Qt::QueuedConnection);
  connect(this, &AbstractDialog::sig_updateProgressMinimum,
          this, &AbstractDialog::updateProgressMinimum_, Qt::QueuedConnection);
  connect(this, &AbstractDialog::sig_updateProgressMaximum,
          this, &AbstractDialog::updateProgressMaximum_, Qt::QueuedConnection);
  connect(this, &AbstractDialog::sig_updateProgressValue,
          this, &AbstractDialog::updateProgressValue_, Qt::QueuedConnection);
  connect(this, &AbstractDialog::sig_updateProgressLabel,
          this, &AbstractDialog::updateProgressLabel_, Qt::QueuedConnection);
  connect(this, &AbstractDialog::sig_repaintProgressBar,
          this, &AbstractDialog::repaintProgressBar_, Qt::QueuedConnection);

  // Select the first tab by default
  ui_tabs->setCurrentIndex(0);

  // Hide the progress bar/label without changing the dialog layout.
  QSizePolicy labelPolicy = ui_label_prog->sizePolicy();
  labelPolicy.setRetainSizeWhenHidden(true);
  labelPolicy.setHorizontalPolicy(QSizePolicy::Ignored);
  ui_label_prog->setSizePolicy(labelPolicy);
  ui_label_prog->setMinimumWidth(0);

  QSizePolicy progbarPolicy = ui_progbar->sizePolicy();
  progbarPolicy.setRetainSizeWhenHidden(true);
  ui_progbar->setSizePolicy(progbarPolicy);

  ui_label_prog->setVisible(false);
  ui_progbar->setVisible(false);
  // Disable the save button until a session begins
  ui_push_save->setEnabled(false);

  // Resize to a fixed fraction of the available screen so the window looks
  // consistent regardless of OS, display DPI, or Qt version.
  // qBound contains the result so the window is never uncomfortably small on
  // low-res displays or uncomfortably large on big external monitors.
  const QRect avail = QtCompat::primaryScreenAvailableGeometry();
  const int w = qBound(760, qRound(avail.width()  * 0.65), 950);
  const int h = qBound(500, qRound(avail.height() * 0.58), 620);
  resize(w, h);
}

AbstractDialog::~AbstractDialog()
{
  if (m_search)
    m_search->clearPromptHandlers();
  if (m_ownsSearchBase)
    delete m_search;
  delete progTimer;
}

void AbstractDialog::disconnectGUI()
{
  emit tabsDisconnectGUI();
  disconnect(m_search, &SearchBase::sessionStarted, this, &AbstractDialog::updateGUI);
  disconnect(this, &AbstractDialog::sig_updateStatus, this, &AbstractDialog::updateStatus_);
}

void AbstractDialog::lockGUI()
{
  emit tabsLockGUI();
}

void AbstractDialog::updateGUI()
{
  QString mode;
  if (m_search->isReadOnly())
    mode = " (Read-Only)";
  else if (m_search->isSessionActive())
    mode = " (Search)";

  setWindowTitle(QString("%1%2")
                   .arg(m_search->getIDString())
                   .arg(mode));
  emit tabsUpdateGUI();
}

void AbstractDialog::applyProgressUpdate(int value, const QString& label, int min, int max)
{
  if (min >= 0)
    updateProgressMinimum(min);
  if (max >= 0)
    updateProgressMaximum(max);
  if (value >= 0)
    updateProgressValue(value);
  if (!label.isNull())
    updateProgressLabel(label);
}

void AbstractDialog::resumeSession()
{
  QString filename;
  filename = QFileDialog::getOpenFileName(
    this, QString("Load search state settings file"), QDir::homePath(),
    "*.state;;*.*", 0, QFileDialog::DontUseNativeDialog);

  // Give the derived dialog a chance on the GUI thread to decide how the
  //   resume should run before the background thread starts.
  if (!filename.isEmpty() && prepareResumeSession(filename)) {
    QPointer<AbstractDialog> self(this);
    (void)QtConcurrent::run([self, filename]() {
      if (self)
        self->resumeSession_(filename);
    });
  }
}

bool AbstractDialog::prepareResumeSession(const QString& filename)
{
  Q_UNUSED(filename);
  return true;
}

void AbstractDialog::resumeSession_(const QString& filename)
{
  Q_UNUSED(filename);
  stopProgressUpdate();
}

void AbstractDialog::updateStatus_(int opt, int run, int fail, int tot)
{
  ui_label_opt->setText(QString::number(opt));
  ui_label_run->setText(QString::number(run));
  ui_label_fail->setText(QString::number(fail));
  ui_label_tot->setText(QString::number(tot));
}

void AbstractDialog::startProgressUpdate_(const QString& text, int min, int max)
{
  ui_progbar->reset();
  ui_progbar->setRange(min, max);
  ui_progbar->setValue(min);
  setProgressLabelText(ui_label_prog, text);
  ui_progbar->setVisible(true);
  ui_label_prog->setVisible(true);
  repaintProgressBar();
  progTimer->start(1000);
}

void AbstractDialog::stopProgressUpdate_()
{
  ui_progbar->reset();
  ui_label_prog->setText("");
  ui_label_prog->setToolTip(QString());
  ui_progbar->setVisible(false);
  ui_label_prog->setVisible(false);
  progTimer->stop();
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
  setProgressLabelText(ui_label_prog, text);
  repaintProgressBar();
}

void AbstractDialog::repaintProgressBar_()
{
  ui_label_prog->repaint();
  ui_progbar->repaint();
}

void AbstractDialog::showBooleanPromptDialogOnGuiThread(const QString& message, bool* ok)
{
  if (!ok)
    return;

  *ok = QMessageBox::question(this, m_search->getIDString(), message,
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes;
}

void AbstractDialog::showPasswordPromptDialogOnGuiThread(
  const QString& message, QString* newPassword, bool* ok)
{
  if (!newPassword)
    return;

  *newPassword = QInputDialog::getText(this, tr("Need password:"), message, QLineEdit::Password,
                                       QString(), ok);
}

void AbstractDialog::showErrorDialogOnGuiThread(const QString& message)
{
  QMessageBox::critical(this, m_search->getIDString(), message);
}

/// @endcond
}
