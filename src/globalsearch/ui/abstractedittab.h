/**********************************************************************
  AbstractEditTab - Generic tab for editing templates

  Copyright (C) 2009-2011 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef ABSTRACTEDITTAB_H
#define ABSTRACTEDITTAB_H

#include <globalsearch/ui/abstracttab.h>

#include <globalsearch/optbase.h>

#include <QList>
#include <QListWidget>
#include <QStringList>

class QComboBox;
class QLineEdit;
class QPushButton;
class QTextEdit;

namespace GlobalSearch {
class AbstractDialog;
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
  Q_OBJECT

public:
  /**
   * Constructor
   *
   * @param parent AbstractDialog that will use this tab
   * @param p Associated OptBase
   */
  explicit AbstractEditTab(AbstractDialog* parent, OptBase* p);

  /**
   * Destructor
   */
  virtual ~AbstractEditTab() override;

public slots:
  /**
   * Lock GUI elements that shouldn't change once the search begins.
   */
  virtual void lockGUI() override;

  /**
   * Update the queue interface in the GUI.
   */
  virtual void updateGUIQueueInterface();

  /**
   * Update the optimizer in the GUI.
   */
  virtual void updateGUIOptimizer();

  /**
   * Force a refresh of the GUI elements using the internal state.
   */
  virtual void updateGUI() override;

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
   * Get the current opt step index.
   *
   * @return The curent opt step index.
   */
  virtual int getCurrentOptStep() { return ui_list_optStep->currentRow(); }

  /**
   * Get the optimizer for the currently selected step.
   *
   * @return The optimizer for the currently selected step.
   */
  virtual Optimizer* getCurrentOptimizer()
  {
    return (getCurrentOptStep() < 0 ? nullptr
                                    : m_opt->optimizer(getCurrentOptStep()));
  }

  /**
   * Get the queue interface for the currently selected step.
   *
   * @return The queue interface for the currently selected step.
   */
  virtual QueueInterface* getCurrentQueueInterface()
  {
    return (getCurrentOptStep() < 0 ? nullptr : m_opt->queueInterface(
                                                  getCurrentOptStep()));
  }

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
   * @return A list of the available template names for the QueueInterface
   *         and Optimizer at a particular opt step.
   */
  virtual QStringList getTemplateNames(size_t optStep);

signals:
  /**
   * Emitted when the Optimizer changes.
   */
  void optimizerChanged(size_t optStep, const std::string&);

  /**
   * Emitted when the QueueInterface changes.
   */
  void queueInterfaceChanged(size_t optStep, const std::string&);

protected slots:
  /**
   * Create connections and initialize GUI.
   */
  virtual void initialize() override;

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
  virtual void updateOptimizer();

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
  QStringList m_optimizers;

  /// List of all QueueInterfaces. This must be filled in derived classes
  /// prior to calling initialize()
  QStringList m_queueInterfaces;

  /// Cached GUI pointer. This is set in DefaultEditTab
  QComboBox* ui_combo_queueInterfaces;
  /// Cached GUI pointer. This is set in DefaultEditTab
  QComboBox* ui_combo_optimizers;
  /// Cached GUI pointer. This is set in DefaultEditTab
  QComboBox* ui_combo_templates;
  /// Cached GUI pointer. This is set in DefaultEditTab
  QLineEdit* ui_edit_user1;
  /// Cached GUI pointer. This is set in DefaultEditTab
  QLineEdit* ui_edit_user2;
  /// Cached GUI pointer. This is set in DefaultEditTab
  QLineEdit* ui_edit_user3;
  /// Cached GUI pointer. This is set in DefaultEditTab
  QLineEdit* ui_edit_user4;
  /// Cached GUI pointer. This is set in DefaultEditTab
  QListWidget* ui_list_edit;
  /// Cached GUI pointer. This is set in DefaultEditTab
  QListWidget* ui_list_optStep;
  /// Cached GUI pointer. This is set in DefaultEditTab
  QPushButton* ui_push_add;
  /// Cached GUI pointer. This is set in DefaultEditTab
  QPushButton* ui_push_help;
  /// Cached GUI pointer. This is set in DefaultEditTab
  QPushButton* ui_push_loadScheme;
  /// Cached GUI pointer. This is set in DefaultEditTab
  QPushButton* ui_push_optimizerConfig;
  /// Cached GUI pointer. This is set in DefaultEditTab
  QPushButton* ui_push_queueInterfaceConfig;
  /// Cached GUI pointer. This is set in DefaultEditTab
  QPushButton* ui_push_remove;
  /// Cached GUI pointer. This is set in DefaultEditTab
  QPushButton* ui_push_saveScheme;
  /// Cached GUI pointer. This is set in DefaultEditTab
  QTextEdit* ui_edit_edit;
};
}

#endif
