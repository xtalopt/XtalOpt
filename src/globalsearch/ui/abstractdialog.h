/**********************************************************************
  AbstractDialog -- A base dialog class for use with libglobalsearch
  projects. See the accompanying .ui file for a Qt Designer template.

  Copyright (C) 2010-2011 by David Lonie

  This library is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as
  published by the Free Software Foundation; either version 2.1 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public icense for more details.
 ***********************************************************************/

#ifndef ABSTRACTDIALOG_H
#define ABSTRACTDIALOG_H

#include <globalsearch/optimizer.h>
#include <globalsearch/optbase.h>

#include <QtGui/QProgressBar>
#include <QtGui/QMessageBox>
#include <QtGui/QTabWidget>
#include <QtGui/QDialog>

#include <QtCore/QMutex>
#include <QtCore/QTimer>

namespace Avogadro {
  class GLWidget;
  class Molecule;
}

namespace GlobalSearch {

  /**
   * @class AbstractDialog abstractdialog.h <globalsearch/ui/abstractdialog.h>
   *
   * @brief A basic dialog that is preconfigured for use with
   * GlobalSearch.
   *
   * @author David C. Lonie
   *
   * AbstractDialog is set up for use with an OptBase class. See the
   * accompanying .ui file for a QtDesigner template.
   *
   * To properly use this class, modify abstractdialog.ui in Qt
   * Designer without changing the names of any existing elements. Do
   * not add tabs, this will be done later programmatically.
   *
   * Include a block like this in the derived class's constructor to setup the UI:
@verbatim
    // Initialize UI
    ui.setupUi(this);
    ui_push_begin   = ui.push_begin;
    ui_push_save    = ui.push_save;
    ui_push_resume  = ui.push_resume;
    ui_label_opt    = ui.label_opt;
    ui_label_run    = ui.label_run;
    ui_label_fail   = ui.label_fail;
    ui_label_prog   = ui.label_prog;
    ui_progbar      = ui.progbar;
    ui_tabs         = ui.tabs;

    // Tabs: be sure to include and define in derived header.
    // Initialize tabs (modify as needed)
    m_tab_init     = new TabInit(this, m_opt);
    m_tab_edit     = new TabEdit(this, m_opt);
    m_tab_opt      = new TabOpt(this, m_opt);
    m_tab_sys      = new TabSys(this, m_opt);
    m_tab_progress = new TabProgress(this, m_opt);
    m_tab_plot     = new TabPlot(this, m_opt);
    m_tab_log      = new TabLog(this, m_opt);

    // Populate tab widget (modify as needed)
    ui.tabs->clear();
    ui.tabs->addTab(m_tab_init->getTabWidget(),	    tr("Cell &Initialization"));
    ui.tabs->addTab(m_tab_edit->getTabWidget(),     tr("Optimization &Templates"));
    ui.tabs->addTab(m_tab_opt->getTabWidget(),      tr("&Optimization Settings"));
    ui.tabs->addTab(m_tab_sys->getTabWidget(),      tr("&System Settings"));
    ui.tabs->addTab(m_tab_progress->getTabWidget(), tr("&Progress"));
    ui.tabs->addTab(m_tab_plot->getTabWidget(),     tr("&Plot"));
    ui.tabs->addTab(m_tab_log->getTabWidget(),      tr("&Log"));

    // Select the first tab by default
    ui.tabs->setCurrentIndex(0);

    // Hide the progress bar/label
    ui.label_prog->setVisible(false);
    ui.progbar->setVisible(false);
@endverbatim
   *
   * It is also essential to call initialize() at the end of the
   * derived constructor.
   */
  class AbstractDialog : public QDialog
  {
    Q_OBJECT

  public:
    /**
     * Constructor.
     *
     * When deriving, be sure to call initialize() after initializing
     * m_opt and ui.
     * @sa initialize
     * @param glWidget The GLwidget from the Avogadro instance
     * @param parent Parent object
     * @param f Window flags
     */
    explicit AbstractDialog( Avogadro::GLWidget *glWidget = 0,
                             QWidget *parent = 0,
                             Qt::WindowFlags f = 0 );

    /**
     * Connect m_opt and the ui to the dialog. Call this in the
     * derived class's constructor after initializing m_opt and the
     * private ui_* member variables.
     */
    void initialize();

    /**
     * Destructor. Deletes m_opt.
     *
     * Consider calling something along the lines of
@verbatim
    if (m_opt->saveOnExit) {
      m_opt->tracker()->lockForRead();
      writeSettings();
      saveSession();
      m_opt->tracker()->unlock();
    }
@endverbatim
     * in the derived destructor.
     */
    virtual ~AbstractDialog();

    /**
     * @return The GLWidget of the main Avogadro window.
     */
    Avogadro::GLWidget* getGLWidget() {return m_glWidget;};

    /**
     * @return The associated OptBase derived class.
     */
    OptBase* getOptBase() {return m_opt;};

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
     * Refresh the GUI from data stored in m_opt.
     * @note This call is passed on to all tabs.
     */
    virtual void updateGUI();

    /**
     * Write persistant settings or resume information. If the
     * filename is omitted, settings are written to the Avogadro
     * configuration file. Otherwise, they are written to the provided
     * file.
     *
     * @note This call is passed on to all tabs.
     * @param filename Optional filename to hold resume information
     */
    virtual void writeSettings(const QString &filename = "") {
      reemitTabsWriteSettings(filename);};

    /**
     * Read persistant settings or resume information. If the filename
     * is omitted, settings are read from the Avogadro configuration
     * file. Otherwise, they are read from the provided file.
     *
     * @note This call is passed on to all tabs.
     * @param filename Optional filename to holding resume information
     */
    virtual void readSettings(const QString &filename = "")
    {
      this->reemitTabsReadSettings(filename);
    };

    /**
     * Saves resume information to a state file in OptBase::filePath.
     *
     * This should look something like:
@verbatim
  void DerivedDialog::saveSession() {
    // Notify if this was user requested.
    if (m_opt->savePending) {
      return;
    }
    bool notify = false;
    if (sender() == ui_push_save) {
      notify = true;
    }
    m_opt->savePending = true;
    QtConcurrent::run(m_opt, &DerivedOptBase::save, QString(""), notify);
  }
@endverbatim
     */
    virtual void saveSession() =0;

    /**
     * Update the GUI with how many Structures are optimized, running,
     * or failing.
     *
     * @param opt Number of optimized structures
     * @param run Number of running structures
     * @param fail Number of failing structures
     */
    void updateStatus(int opt, int run, int fail) {
      emit sig_updateStatus(opt,run,fail);};

    /**
     * Update the cached Avogadro::GLWidget pointer
     *
     * @param w The Avogadro GLWidget
     */
    void setGLWidget(Avogadro::GLWidget *w) {m_glWidget = w;};

    /**
     * @name Progressbar functions
     * These functions are used to display and control a progress
     * notification system for log processes.
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
     * @return False if the progress bar is already in use, true otherwise.
     */
    bool startProgressUpdate(const QString & text, int min, int max);

    /**
     * Reset and hide progress bar and label. Also frees the
     * associated mutex, allowing other processes to use it.
     */
    void stopProgressUpdate();

    /**
     * @param min The minimum value for the progress bar.
     */
    void updateProgressMinimum(int min) {
      emit sig_updateProgressMinimum(min);};

    /**
     * @param max The maximum value for the progress bar.
     */
    void updateProgressMaximum(int max) {
      emit sig_updateProgressMaximum(max);};

    /**
     * @param val The current value for the progress bar.
     */
    void updateProgressValue(int val) {
      emit sig_updateProgressValue(val);};

    /**
     * @param text The text for the progress label.
     */
    void updateProgressLabel(const QString & text) {
      emit sig_updateProgressLabel(text);};

    /**
     * Forces a redraw of the progress bar.
     *
     * @note This shouldn't need to be called, as it is handled
     * automatically.
     */
    void repaintProgressBar() {
      emit sig_repaintProgressBar();};
    /** @} */

    /**
     * Called by OptBase::debug and sends a message to the log tab.
     *
     * @note Do not use this function, but instead send call
     * OptBase::debug
     *
     * @param s The debugging message.
     */
    void newDebug(const QString &s) {emit newLog("Debug: " + s);};

    /**
     * Called by OptBase::warning and sends a message to the log tab.
     *
     * @note Do not use this function, but instead send call
     * OptBase::warning
     *
     * @param s The warning message.
     */
    void newWarning(const QString &s) {emit newLog("Warning: " + s);};

    /**
     * Called by OptBase::error. Sends a message to the log tab
     * and calls errorBox.
     *
     * @note Do not use this function, but instead send call
     * OptBase::error
     *
     * @param s The error message.
     */
    void newError(const QString &s) {
      emit newLog("Error: " + s);  errorBox(s);};

    /**
     * Displays an error box with the indicated message. This function
     * will block the GUI thread until user clicks "Ok".
     *
     * @note Do not use this function, but instead send call
     * OptBase::error
     *
     * @param s Error message.
     */
    void errorBox(const QString &s) {emit sig_errorBox(s);};

  protected slots:
    /**
     * Begin the search. Suggested form for derived class:
@verbatim
  void DerivedDialog::startSearch() {
    QtConcurrent::run(m_opt, &DerivedOptBase::startSearch);
  }
@endverbatim
     */
    virtual void startSearch() =0;

    /**
     * Prompt user for a resume file and then call resumeSession_ in a
     * background thread.
     */
    virtual void resumeSession();

    /**
     * Hidden call. Ensures that the GUI is modified from the
     * appropriate thread.
     * @sa updateStatus
     */
    void updateStatus_(int, int, int);

    /**
     * Hidden call. Ensures that the GUI is modified from the
     * appropriate thread.
     * @sa startProgressUpdate
     */
    void startProgressUpdate_(const QString &, int, int);

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
    void updateProgressLabel_(const QString &);

    /**
     * Hidden call. Ensures that the GUI is modified from the
     * appropriate thread.
     * @sa repaintProgressBar
     */
    void repaintProgressBar_();

    /**
     * Hidden call. Ensures that the GUI is modified from the
     * appropriate thread.
     * @sa errorBox
     */
    void errorBox_(const QString &s) {
      QMessageBox::critical(this, "Error", s);};

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
     * Emitted when tabs should run their updateGUI function
     */
    void tabsUpdateGUI();

    /**
     * Emitted to change/update the molecule displayed in the Avogadro
     * main window.
     */
    void moleculeChanged(GlobalSearch::Structure*);

    /**
     * Emitted when there is a new log message ready.
     * @sa OptBase::debug
     * @sa OptBase::warning
     * @sa OptBase::error
     * @param str Log message
     */
    void newLog(const QString &str);

    /**
     * Hidden call. Ensures that the GUI is modified from the
     * appropriate thread.
     * @sa updateStatus
     */
    void sig_updateStatus(int,int,int);

    /**
     * Hidden call. Ensures that the GUI is modified from the
     * appropriate thread.
     * @sa startProgressUpdate
     */
    void sig_startProgressUpdate(const QString &, int, int);

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
    void sig_updateProgressLabel(const QString &);

    /**
     * Hidden call. Ensures that the GUI is modified from the
     * appropriate thread.
     * @sa repaintProgressBar
     */
    void sig_repaintProgressBar();

    /**
     * Hidden call. Ensures that the GUI is modified from the
     * appropriate thread.
     * @sa errorBox
     */
    void sig_errorBox(const QString &);

  protected:
    /**
     * Resumes the session in file \a filename.
     */
    virtual void resumeSession_(const QString &filename);

    /**
     * Cached pointer to the associated OptBase object.
     */
    OptBase *m_opt;

    /**
     * The molecule object in the main Avogadro window.
     */
    Avogadro::Molecule *m_molecule;

    /**
     * Cached pointer to the Avogadro GLWidget.
     */
    Avogadro::GLWidget *m_glWidget;

    /**
     * Mutex governing progress bar usage.
     */
    QMutex *progMutex;

    /**
     * Timer to automatically refresh the progress bar.
     */
    QTimer *progTimer;

    /**
     * Pointer to GUI element. Do not use in derived class code.
     * @note This must be set up in the derived-constructor. See class
     * description.
     */
    QPushButton *ui_push_begin;

    /**
     * Pointer to GUI element. Do not use in derived class code.
     * @note This must be set up in the derived-constructor. See class
     * description.
     */
    QPushButton *ui_push_save;

    /**
     * Pointer to GUI element. Do not use in derived class code.
     * @note This must be set up in the derived-constructor. See class
     * description.
     */
    QPushButton *ui_push_resume;

    /**
     * Pointer to GUI element. Do not use in derived class code.
     * @note This must be set up in the derived-constructor. See class
     * description.
     */
    QLabel *ui_label_opt;

    /**
     * Pointer to GUI element. Do not use in derived class code.
     * @note This must be set up in the derived-constructor. See class
     * description.
     */
    QLabel *ui_label_run;

    /**
     * Pointer to GUI element. Do not use in derived class code.
     * @note This must be set up in the derived-constructor. See class
     * description.
     */
    QLabel *ui_label_fail;

    /**
     * Pointer to GUI element. Do not use in derived class code.
     * @note This must be set up in the derived-constructor. See class
     * description.
     */
    QLabel *ui_label_prog;

    /**
     * Pointer to GUI element. Do not use in derived class code.
     * @note This must be set up in the derived-constructor. See class
     * description.
     */
    QProgressBar *ui_progbar;

    /**
     * Pointer to GUI element. Do not use in derived class code.
     * @note This must be set up in the derived-constructor. See class
     * description.
     */
    QTabWidget *ui_tabs;

    /// @cond
    // These are used to ensure that all tab*settings signals are
    // handled immediately, using direct or blocking connections as
    // needed.
  signals:
    void tabsWriteSettingsBlockingQueued(const QString &filename);
    void tabsWriteSettingsDirect(const QString &filename);
    void tabsReadSettingsBlockingQueued(const QString &filename);
    void tabsReadSettingsDirect(const QString &filename);
  private slots:
    void reemitTabsWriteSettings(const QString &filename);
    void reemitTabsReadSettings(const QString &filename);
    /// @endcond


  };
}

#endif
