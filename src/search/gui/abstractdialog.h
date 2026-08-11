/**********************************************************************
  AbstractDialog - A base dialog class for use with libsearch
  projects. See the accompanying .ui file for a Qt Designer template.

  Copyright (C) 2010-2011 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef ABSTRACTDIALOG_H
#define ABSTRACTDIALOG_H

#include <QDialog>
#include <QMessageBox>

class QProgressBar;
class QTabWidget;
class QTimer;

namespace Search {
class SearchBase;
class Structure;

/**
 * @class AbstractDialog abstractdialog.h <search/gui/abstractdialog.h>
 *
 * @brief A ready-made dialog for use with a Search.
 *
 * @author David C. Lonie
 *
 * AbstractDialog pairs with an SearchBase class and the accompanying .ui file.
 * Edit abstractdialog.ui in Qt Designer, but don't rename any existing element,
 * and don't add tabs there - we add those in code.
 *
 * In your derived constructor, wire up the GUI with a block like this:
@verbatim
  // Initialize GUI
  ui.setupUi(this);
  ui_push_begin   = ui.push_begin;
  ui_push_save    = ui.push_save;
  ui_push_resume  = ui.push_resume;
  ui_label_opt    = ui.label_opt;
  ui_label_run    = ui.label_run;
  ui_label_tot    = ui.label_tot;
  ui_label_fail   = ui.label_fail;
  ui_label_prog   = ui.label_prog;
  ui_progbar      = ui.progbar;
  ui_tabs         = ui.tabs;

  // Tabs: be sure to include and define in derived header.
  // Initialize tabs (modify as needed)
  m_tab_struc    = new TabStruc(this, m_search);
  m_tab_opt      = new TabOpt(this, m_search);
  m_tab_search   = new TabSearch(this, m_search);
  m_tab_progress = new TabProgress(this, m_search);
  m_tab_plot     = new TabPlot(this, m_search);
  m_tab_log      = new TabLog(this, m_search);
  m_tab_about    = new TabAbout(this, m_search);

  // Populate tab widget (modify as needed)
  ui.tabs->clear();
  ui.tabs->addTab(m_tab_struc->getTabWidget(),    tr("Cell &Initialization"));
  ui.tabs->addTab(m_tab_opt->getTabWidget(),      tr("Optimization &Templates"));
  ui.tabs->addTab(m_tab_search->getTabWidget(),   tr("&Optimization Settings"));
  ui.tabs->addTab(m_tab_progress->getTabWidget(), tr("&Progress"));
  ui.tabs->addTab(m_tab_plot->getTabWidget(),     tr("&Plot"));
  ui.tabs->addTab(m_tab_log->getTabWidget(),      tr("&Log"));
  ui.tabs->addTab(m_tab_about->getTabWidget(),    tr("&About"));

  // Select the first tab by default
  ui.tabs->setCurrentIndex(0);

  // Hide the progress bar/label
  ui.label_prog->setVisible(false);
  ui.progbar->setVisible(false);
@endverbatim
 *
 * And don't forget to call initialize() at the very end of the constructor.
 */
class AbstractDialog : public QDialog
{
  Q_OBJECT

public:
  /**
   * Constructor.
   *
   * When deriving, be sure to call initialize() after initializing
   * m_search and ui.
   * @sa initialize
   * @param parent Parent object
   * @param f Window flags
   */
  explicit AbstractDialog(QWidget* parent = 0, Qt::WindowFlags f = Qt::Window);

  /**
   * Connect m_search and the ui to the dialog. Call this in the
   * derived class's constructor after initializing m_search and the
   * private ui_* member variables.
   */
  void initialize();

  /**
   * Destructor. Deletes m_search.
   *
   * If a final save is needed on exit, do it in the derived application
   * object, not here.
   */
  virtual ~AbstractDialog() override;

  /**
   * @return The associated SearchBase derived class.
   */
  SearchBase* getSearchBase() { return m_search; };

public slots:
  /**
   * Call this to disable GUI updates. Useful when benchmarking
   * non-interactively.
   * @note This call is passed on to all tabs.
   */
  virtual void disconnectGUI();

  /**
   * Called when the search session starts to disable GUI components
   * that should only be modified during initialization.
   * @note This call is passed on to all tabs.
   */
  virtual void lockGUI();

  /**
   * Refresh the GUI from data stored in m_search.
   * @note This call is passed on to all tabs.
   */
  virtual void updateGUI();

  /**
   * Show the dialog's progress bar/label when SearchBase starts a
   * progress update.
   */
  void handleProgressStarted(const QString& text, int min, int max)
  {
    startProgressUpdate(text, min, max);
  };

  /**
   * Apply a progress update sent by SearchBase.
   */
  void applyProgressUpdate(int value, const QString& label, int min, int max);

  /**
   * Hide the dialog's progress bar/label when SearchBase ends a
   * progress update.
   */
  void handleProgressEnded() { stopProgressUpdate(); };

  /**
   * Saves resume information. The derived application dialog knows the
   * state file format, so it does the actual save.
   */
  virtual void saveSession() = 0;

  /**
   * Return the settings file used for GUI session state, given a search
   * state file.
   *
   * By default this is the state file itself, as it has always been.
   * Derived dialogs may override this to keep the GUI settings in a
   * separate file.
   */
  virtual QString sessionSettingsFilePath(const QString& stateFile) const
  {
    return stateFile;
  }

  /**
   * Update the GUI with how many Structures are optimized, running,
   * or failing.
   *
   * @param opt Number of optimized structures
   * @param run Number of running structures
   * @param fail Number of failing structures
   */
  void updateStatus(int opt, int run, int fail, int total)
  {
    emit sig_updateStatus(opt, run, fail, total);
  };

protected slots:
  /**
   * Begin the search. Suggested form for derived class:
@verbatim
void DerivedDialog::startSearch() {
  QPointer<DerivedDialog> self(this);
  (void)QtConcurrent::run([self]() {
    if (self)
      self->m_search->startSearch();
  });
}
@endverbatim
   */
  virtual void startSearch() = 0;

  /**
   * Prompt user for a resume file and then call resumeSession_ in a
   * background thread.
   */
  virtual void resumeSession();

  /**
   * @name Progressbar functions
   * These functions display and control the dialog's progress
   * notification. Derived dialogs may use them directly; tabs don't
   * call them.
   * @{
   */

  /**
   * Show the progressbar and initialize a status update.
   *
   * Only one progress update may run at a time.
   *
   * @param text Label text describing the operation
   * @param min Minimum progress value
   * @param max Maximum progress value
   */
  void startProgressUpdate(const QString& text, int min, int max)
  {
    emit sig_startProgressUpdate(text, min, max);
  };

  /**
   * Reset and hide progress bar and label. Also frees the
   * associated mutex, allowing other processes to use it.
   */
  void stopProgressUpdate() { emit sig_stopProgressUpdate(); };

  /**
   * @param min The minimum value for the progress bar.
   */
  void updateProgressMinimum(int min) { emit sig_updateProgressMinimum(min); };

  /**
   * @param max The maximum value for the progress bar.
   */
  void updateProgressMaximum(int max) { emit sig_updateProgressMaximum(max); };

  /**
   * @param val The current value for the progress bar.
   */
  void updateProgressValue(int val) { emit sig_updateProgressValue(val); };

  /**
   * @param text The text for the progress label.
   */
  void updateProgressLabel(const QString& text)
  {
    emit sig_updateProgressLabel(text);
  };

  /**
   * Forces a redraw of the progress bar.
   *
   * @note This shouldn't need to be called, as it is handled
   * automatically.
   */
  void repaintProgressBar() { emit sig_repaintProgressBar(); };
  /** @} */

  /**
   * Hidden call. Ensures that the GUI is modified from the
   * appropriate thread.
   * @sa updateStatus
   */
  void updateStatus_(int, int, int, int);

  /**
   * Hidden call. Ensures that the GUI is modified from the
   * appropriate thread.
   * @sa startProgressUpdate
   */
  void startProgressUpdate_(const QString&, int, int);

  /**
   * Hidden call. Ensures that the GUI is modified from the
   * appropriate thread.
   * @sa stopProgressUpdate
   */
  void stopProgressUpdate_();

  /**
   * Hidden call. Ensures that the GUI is modified from the
   * appropriate thread.
   * @sa updateProgressMinimum
   */
  void updateProgressMinimum_(int);

  /**
   * Hidden call. Ensures that the GUI is modified from the
   * appropriate thread.
   * @sa updateProgressMaximum
   */
  void updateProgressMaximum_(int);

  /**
   * Hidden call. Ensures that the GUI is modified from the
   * appropriate thread.
   * @sa updateProgressValue
   */
  void updateProgressValue_(int);

  /**
   * Hidden call. Ensures that the GUI is modified from the
   * appropriate thread.
   * @sa updateProgressLabel
   */
  void updateProgressLabel_(const QString&);

  /**
   * Hidden call. Ensures that the GUI is modified from the
   * appropriate thread.
   * @sa repaintProgressBar
   */
  void repaintProgressBar_();

  /**
   * Hidden call. Ensures that prompts are handled on the GUI thread.
   */
  void showBooleanPromptDialogOnGuiThread(const QString& message, bool* ok);

  /**
   * Hidden call. Ensures that prompts are handled on the GUI thread.
   */
  void showPasswordPromptDialogOnGuiThread(const QString& message, QString* newPassword, bool* ok);

  /**
   * Hidden call. Ensures that error notifications are handled on the GUI
   * thread.
   */
  void showErrorDialogOnGuiThread(const QString& message);

signals:
  /**
   * Emitted when tabs should run their disconnectGUI function
   */
  void tabsDisconnectGUI();

  /**
   * Emitted when tabs should run their lockGUI function
   */
  void tabsLockGUI();

  /**
   * Emitted when tabs should run their lockGUI function
   */
  void tabsUpdateGUI();

  /**
   * Emitted when there is a new log message ready.
   * @sa Common::debug
   * @sa Common::warning
   * @sa Common::error
   * @param str Log message
   */
  void newLog(const QString& str);

  /**
   * Hidden call. Ensures that the GUI is modified from the
   * appropriate thread.
   * @sa updateStatus
   */
  void sig_updateStatus(int, int, int, int);

  /**
   * Hidden call. Ensures that the GUI is modified from the
   * appropriate thread.
   * @sa startProgressUpdate
   */
  void sig_startProgressUpdate(const QString&, int, int);

  /**
   * Hidden call. Ensures that the GUI is modified from the
   * appropriate thread.
   * @sa stopProgressUpdate
   */
  void sig_stopProgressUpdate();

  /**
   * Hidden call. Ensures that the GUI is modified from the
   * appropriate thread.
   * @sa updateProgressMinimum
   */
  void sig_updateProgressMinimum(int);

  /**
   * Hidden call. Ensures that the GUI is modified from the
   * appropriate thread.
   * @sa updateProgressMaximum
   */
  void sig_updateProgressMaximum(int);

  /**
   * Hidden call. Ensures that the GUI is modified from the
   * appropriate thread.
   * @sa updateProgressValue
   */
  void sig_updateProgressValue(int);

  /**
   * Hidden call. Ensures that the GUI is modified from the
   * appropriate thread.
   * @sa updateProgressLabel
   */
  void sig_updateProgressLabel(const QString&);

  /**
   * Hidden call. Ensures that the GUI is modified from the
   * appropriate thread.
   * @sa repaintProgressBar
   */
  void sig_repaintProgressBar();

protected:
  /**
   * Called on the GUI thread after a resume file is selected, before the
   * resume starts in the background. Derived application dialogs can
   * decide here how the resume should run, or return false to cancel it.
   */
  virtual bool prepareResumeSession(const QString& filename);

  /**
   * Resumes the session in file \a filename. The derived application dialog
   * handles the actual loading, since it knows the state file format.
   */
  virtual void resumeSession_(const QString& filename);

  /**
   * Cached pointer to the associated SearchBase object.
   */
  SearchBase* m_search;

  /**
   * Whether or not to delete the Searchbase object upon destruction.
   */
  bool m_ownsSearchBase;

  /**
   * Timer to automatically refresh the progress bar.
   */
  QTimer* progTimer;

  /**
   * Pointer to GUI element. Do not use in derived class code.
   * @note This must be set up in the derived-constructor. See class
   * description.
   */
  QPushButton* ui_push_begin;

  /**
   * Pointer to GUI element. Do not use in derived class code.
   * @note This must be set up in the derived-constructor. See class
   * description.
   */
  QPushButton* ui_push_save;

  /**
   * Pointer to GUI element. Do not use in derived class code.
   * @note This must be set up in the derived-constructor. See class
   * description.
   */
  QPushButton* ui_push_resume;

  /**
   * Pointer to GUI element. Do not use in derived class code.
   * @note This must be set up in the derived-constructor. See class
   * description.
   */
  QLabel* ui_label_opt;

  /**
   * Pointer to GUI element. Do not use in derived class code.
   * @note This must be set up in the derived-constructor. See class
   * description.
   */
  QLabel* ui_label_run;

  /**
   * Pointer to GUI element. Do not use in derived class code.
   * @note This must be set up in the derived-constructor. See class
   * description.
   */
  QLabel* ui_label_tot;

  /**
   * Pointer to GUI element. Do not use in derived class code.
   * @note This must be set up in the derived-constructor. See class
   * description.
   */
  QLabel* ui_label_fail;

  /**
   * Pointer to GUI element. Do not use in derived class code.
   * @note This must be set up in the derived-constructor. See class
   * description.
   */
  QLabel* ui_label_prog;

  /**
   * Pointer to GUI element. Do not use in derived class code.
   * @note This must be set up in the derived-constructor. See class
   * description.
   */
  QProgressBar* ui_progbar;

  /**
   * Pointer to GUI element. Do not use in derived class code.
   * @note This must be set up in the derived-constructor. See class
   * description.
   */
  QTabWidget* ui_tabs;

};
}

#endif
