/**********************************************************************
  AbstractTab -- Basic GlobalSearch tab functionality

  Copyright (C) 2009-2011 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef ABSTRACTTAB_H
#define ABSTRACTTAB_H

#include <atomic>

#include <QObject>

namespace GlobalSearch {
class AbstractDialog;
class Structure;
class OptBase;

/**
 * @class AbstractTab abstracttab.h <globalsearch/ui/abstracttab.h>
 *
 * @brief The base class for UI tabs, preconfigured to work with
 * dialogs derived from AbstractDialog.
 *
 * @author David C. Lonie
 */
class AbstractTab : public QObject
{
  Q_OBJECT

public:
  /**
   * Constructor
   *
   * Minimum constructor for derived tab:
@verbatim
DerivedTab::DerivedTab(AbstractDialog *parent,
                       OptBase *p) :
  AbstractTab(parent, p)
{
ui.setupUi(m_tab_widget);

initialize();
}
@endverbatim
   */
  explicit AbstractTab(AbstractDialog* parent, OptBase* p);

  /**
   * Destructor
   */
  virtual ~AbstractTab() override;

  /**
   * @return The widget for this tab.
   */
  QWidget* getTabWidget() { return m_tab_widget; };

public slots:
  /**
   * Disable portions of the GUI that shouldn't be modified after
   * the search has started.
   */
  virtual void lockGUI(){};

  /**
   * Load any parameters that this tab is responible for here.
   * @note In most cases, this shouldn't be called directly, but
   * rather call the same function in the parent dialog class.
   * @param filename If specified, read from given file. Otherwise
   * read from system config file.
   */
  virtual void readSettings(const QString& filename = "")
  {
    Q_UNUSED(filename);
  };

  /**
   * Save any parameters that this tab is responible for here.
   * @note In most cases, this shouldn't be called directly, but
   * rather call the same function in the parent dialog class.
   * @param filename If specified, write to given file. Otherwise
   * write to system config file.
   */
  virtual void writeSettings(const QString& filename = "")
  {
    Q_UNUSED(filename);
  };

  /**
   * Set any GUI elements from information in internal data
   * structures.
   */
  virtual void updateGUI(){};

  /**
   * Used during benchmarking to disable any GUI update connections.
   */
  virtual void disconnectGUI(){};

protected slots:
  /**
   * Create some default connections between the main dialog and
   * this tab.
   */
  virtual void initialize();

  /**
   * Sets the application's busy cursor. This should not be called
   * directly, instead emit startingBackgroundProcessing(), which is
   * safe to use from a background thread.
   *
   * @sa startingBackgroundProcessing()
   * @sa finishedBackgroundProcessing()
   */
  void setBusyCursor();

  /**
   * Resets the application's busy cursor. This should not be called
   * directly, instead emit finishedBackgroundProcessing(), which is
   * safe to use from a background thread.
   *
   * @sa startingBackgroundProcessing()
   * @sa finishedBackgroundProcessing()
   */
  void clearBusyCursor();

signals:
  /**
   * Emit to update the molecule displayed in the Avogadro GLWidget
   */
  void moleculeChanged(GlobalSearch::Structure*);

  /**
   * Emit this signal before beginning user-requested
   * processing. This will set the busy cursor in the application.
   */
  void startingBackgroundProcessing();

  /**
   * Emit this signal after user-requested processing has
   * completed. This will reset the busy cursor in the application.
   */
  void finishedBackgroundProcessing();

  /**
   * Emitted when initialize completes
   */
  void initialized();

protected:
  /// The actual widget that will be made into a tab.
  QWidget* m_tab_widget;

  /// A pointer to the parent dialog.
  AbstractDialog* m_dialog;

  /// A pointer to the associated OptBase class.
  OptBase* m_opt;

  /// Set to true once initialized() completes.
  bool m_isInitialized;

  /// Used to determine if a gui update is in progress
  std::atomic<bool> m_updateGuiInProgress{false};
};
}

#endif
