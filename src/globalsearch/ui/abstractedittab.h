/**********************************************************************
  AbstractEditTab - Generic tab for editing templates

  Copyright (C) 2009-2011 by David Lonie

  This library is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as
  published by the Free Software Foundation; either version 2.1 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public icense for more details.
 ***********************************************************************/

#ifndef ABSTRACTEDITTAB_H
#define ABSTRACTEDITTAB_H

#include <globalsearch/ui/abstracttab.h>

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QStringList>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QListWidget;
class QPushButton;
class QTextEdit;

namespace GlobalSearch {
  class AbstractDialog;
  class OptBase;
  class Optimizer;
  class QueueInterface;

  /**
   * @class AbstractEditTab abstractedittab.h <globalsearch/abstractedittab.h>
   *
   * @brief Abstract class implementing a template editor
   *
   * @author David C. Lonie
   */
  class AbstractEditTab : public GlobalSearch::AbstractTab
  {
    Q_OBJECT;

  public:
    /**
     * Constructor
     *
     * @param parent AbstractDialog that will use this tab
     * @param p Associated OptBase
     */
    explicit AbstractEditTab( AbstractDialog *parent, OptBase *p );

    /**
     * Destructor
     */
    virtual ~AbstractEditTab();

  public slots:
    /**
     * Lock GUI elements that shouldn't change once the search begins.
     */
    virtual void lockGUI();

    /**
     * Force a refresh of the GUI elements using the internal state.
     */
    virtual void updateGUI();

    /**
     * Display the currently selected template in the text editor.
     */
    virtual void updateEditWidget();

    /**
     * Popup a message box displaying the keyword documentation.
     */
    virtual void showHelp();

    /**
     * Save the text in the template editor to the appropriate
     * template list.
     */
    virtual void saveCurrentTemplate();

    /**
     * Generate the list of optsteps.
     */
    virtual void populateOptStepList();

    /**
     * Fill the template selection combo using the template names for
     * the current QueueInterface and Optimizer.
     */
    virtual void populateTemplates();

    /**
     * Create a new optstep at the end of the optstep list. It will
     * initialize using the currently selected optstep's templates.
     */
    virtual void appendOptStep();

    /**
     * Delete the currently selected optstep.
     */
    virtual void removeCurrentOptStep();

    /**
     * Save the current optimization scheme. This will prompt for the
     * user to specify the filename.
     */
    virtual void saveScheme();

    /**
     * Load an optimization scheme from a file. This will prompt the
     * user for the filename.
     */
    virtual void loadScheme();

    /**
     * @return A list of the available template names for the current
     * QueueInterface and Optimizer.
     */
    virtual QStringList getTemplateNames();

  signals:
    /**
     * Emitted when the Optimizer changes.
     */
    void optimizerChanged(Optimizer*);

    /**
     * Emitted when the QueueInterface changes.
     */
    void queueInterfaceChanged(QueueInterface*);

  protected slots:
    /**
     * Create connections and initialize GUI.
     */
    virtual void initialize();

    /**
     * Refresh the "userX" line edits.
     */
    virtual void updateUserValues();

    /**
     * Determine the currently selected QueueInterface and emit
     * queueInterfaceChanged if it differs from the current one.
     */
    virtual void updateQueueInterface();

    /**
     * Determine the currently selected Optimizer and emit
     * optimizerChanged if it differs from the current one.
     */
    virtual void updateOptimizer();\

    /**
     * Launch the QueueInterface configuration dialog.
     */
    virtual void configureQueueInterface();

    /**
     * Launch the Optimizer configuration dialog.
     */
    virtual void configureOptimizer();

  protected:
    /// List of all optimizers. This must be filled in derived classes
    /// prior to calling initialize()
    QList<Optimizer*> m_optimizers;

    /// List of all QueueInterfaces. This must be filled in derived classes
    /// prior to calling initialize()
    QList<QueueInterface*> m_queueInterfaces;

    /// Cached GUI pointer. This is set in DefaultEditTab
    QCheckBox     *ui_cb_preopt;
    /// Cached GUI pointer. This is set in DefaultEditTab
    QComboBox     *ui_combo_queueInterfaces;
    /// Cached GUI pointer. This is set in DefaultEditTab
    QComboBox     *ui_combo_optimizers;
    /// Cached GUI pointer. This is set in DefaultEditTab
    QComboBox     *ui_combo_templates;
    /// Cached GUI pointer. This is set in DefaultEditTab
    QLineEdit     *ui_edit_user1;
    /// Cached GUI pointer. This is set in DefaultEditTab
    QLineEdit     *ui_edit_user2;
    /// Cached GUI pointer. This is set in DefaultEditTab
    QLineEdit     *ui_edit_user3;
    /// Cached GUI pointer. This is set in DefaultEditTab
    QLineEdit     *ui_edit_user4;
    /// Cached GUI pointer. This is set in DefaultEditTab
    QListWidget   *ui_list_edit;
    /// Cached GUI pointer. This is set in DefaultEditTab
    QListWidget   *ui_list_optStep;
    /// Cached GUI pointer. This is set in DefaultEditTab
    QPushButton   *ui_push_add;
    /// Cached GUI pointer. This is set in DefaultEditTab
    QPushButton   *ui_push_help;
    /// Cached GUI pointer. This is set in DefaultEditTab
    QPushButton   *ui_push_loadScheme;
    /// Cached GUI pointer. This is set in DefaultEditTab
    QPushButton   *ui_push_optimizerConfig;
    /// Cached GUI pointer. This is set in DefaultEditTab
    QPushButton   *ui_push_preoptConfig;
    /// Cached GUI pointer. This is set in DefaultEditTab
    QPushButton   *ui_push_queueInterfaceConfig;
    /// Cached GUI pointer. This is set in DefaultEditTab
    QPushButton   *ui_push_remove;
    /// Cached GUI pointer. This is set in DefaultEditTab
    QPushButton   *ui_push_saveScheme;
    /// Cached GUI pointer. This is set in DefaultEditTab
    QTextEdit     *ui_edit_edit;
  };
}

#endif
