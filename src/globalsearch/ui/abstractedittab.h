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

  class AbstractEditTab : public GlobalSearch::AbstractTab
  {
    Q_OBJECT;

  public:
    explicit AbstractEditTab( AbstractDialog *parent, OptBase *p );
    virtual ~AbstractEditTab();

  public slots:
    virtual void lockGUI();
    virtual void updateGUI();
    virtual void updateEditWidget();
    virtual void showHelp();
    virtual void saveCurrentTemplate();
    virtual void populateOptStepList();
    virtual void populateTemplates();
    virtual void appendOptStep();
    virtual void removeCurrentOptStep();
    virtual void saveScheme();
    virtual void loadScheme();
    virtual QStringList getTemplateNames();

  signals:
    void optimizerChanged(Optimizer*);
    void queueInterfaceChanged(QueueInterface*);

  protected slots:
    virtual void initialize();
    virtual void updateUserValues();
    virtual void updateQueueInterface();
    virtual void updateOptimizer();
    virtual void configureQueueInterface();
    virtual void configureOptimizer();

  protected:
    QList<Optimizer*> m_optimizers;
    QList<QueueInterface*> m_queueInterfaces;

    // Cached GUI pointers for setup
    QComboBox     *ui_combo_queueInterfaces;
    QComboBox     *ui_combo_optimizers;
    QComboBox     *ui_combo_templates;
    QLineEdit     *ui_edit_user1, *ui_edit_user2, *ui_edit_user3, *ui_edit_user4;
    QListWidget   *ui_list_edit;
    QListWidget   *ui_list_optStep;
    QPushButton   *ui_push_add;
    QPushButton   *ui_push_help;
    QPushButton   *ui_push_loadScheme;
    QPushButton   *ui_push_optimizerConfig;
    QPushButton   *ui_push_queueInterfaceConfig;
    QPushButton   *ui_push_remove;
    QPushButton   *ui_push_saveScheme;
    QTextEdit     *ui_edit_edit;
  };
}

#endif
