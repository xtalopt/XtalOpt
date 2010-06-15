/**********************************************************************
  AbstractTab -- Basic GlobalSearch tab functionality

  Copyright (C) 2009-2010 by David Lonie

  This library is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as
  published by the Free Software Foundation; either version 2.1 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public icense for more details.
 ***********************************************************************/

#ifndef ABSTRACTTAB_H
#define ABSTRACTTAB_H

#include <QObject>

namespace GlobalSearch {
  class AbstractDialog;
  class Structure;
  class OptBase;

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
    explicit AbstractTab( AbstractDialog *parent, OptBase *p );

    /**
     * Destructor
     */
    virtual ~AbstractTab();

    /**
     * @return The widget for this tab.
     */
    QWidget *getTabWidget() {return m_tab_widget;};

  public slots:
    /**
     * Disable portions of the GUI that shouldn't be modified after
     * the search has started.
     */
    virtual void lockGUI() {};

    /**
     * Load any parameters that this tab is responible for here.
     * @note In most cases, this shouldn't be called directly, but
     * rather call the same function in the parent dialog class.
     * @param filename If specified, read from given file. Otherwise
     * read from system config file.
     */
    virtual void readSettings(const QString &filename = "") {
      Q_UNUSED(filename);};

    /**
     * Save any parameters that this tab is responible for here.
     * @note In most cases, this shouldn't be called directly, but
     * rather call the same function in the parent dialog class.
     * @param filename If specified, write to given file. Otherwise
     * write to system config file.
     */
    virtual void writeSettings(const QString &filename = "") {
      Q_UNUSED(filename);};

    /**
     * Set any GUI elements from information in internal data
     * structures.
     */
    virtual void updateGUI() {};

    /**
     * Used during benchmarking to disable any GUI update connections.
     */
    virtual void disconnectGUI() {};

  protected slots:
    /**
     * Create some default connections between the main dialog and
     * this tab.
     */
    void initialize();

  signals:
    /**
     * Emit to update the molecule displayed in the Avogadro GLWidget
     */
    void moleculeChanged(Structure*);

  protected:
    QWidget *m_tab_widget;
    AbstractDialog *m_dialog;
    OptBase *m_opt;
  };
}

#endif
