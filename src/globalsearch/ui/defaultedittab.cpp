/**********************************************************************
  DefaultEditTab - Simple implementation of AbstractEditTab

  Copyright (C) 2011 by David Lonie

  This library is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as
  published by the Free Software Foundation; either version 2.1 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public icense for more details.
 ***********************************************************************/

#include <globalsearch/ui/defaultedittab.h>

#include "ui_defaultedittab.h"

namespace GlobalSearch {

  DefaultEditTab::DefaultEditTab(AbstractDialog *dialog,
                                 OptBase *opt)
    : AbstractEditTab(dialog, opt),
      ui(new Ui::DefaultEditTab)
  {
    ui->setupUi(m_tab_widget);

    ui_cb_preopt             = ui->cb_preopt;
    ui_combo_queueInterfaces = ui->combo_queueInterfaces;
    ui_combo_optimizers      = ui->combo_optimizers;
    ui_combo_templates       = ui->combo_templates;
    ui_edit_user1            = ui->edit_user1;
    ui_edit_user2            = ui->edit_user2;
    ui_edit_user3            = ui->edit_user3;
    ui_edit_user4            = ui->edit_user4;
    ui_list_edit             = ui->list_edit;
    ui_list_optStep          = ui->list_optStep;
    ui_push_add              = ui->push_add;
    ui_push_help             = ui->push_help;
    ui_push_loadScheme       = ui->push_loadScheme;
    ui_push_optimizerConfig  = ui->push_optimizerConfig;
    ui_push_preoptConfig     = ui->push_preoptConfig;
    ui_push_queueInterfaceConfig
                             = ui->push_queueInterfaceConfig;
    ui_push_remove           = ui->push_remove;
    ui_push_saveScheme       = ui->push_saveScheme;
    ui_edit_edit             = ui->edit_edit;

    // don't show the preopt cb/pushbutton by default. setVisible in
    // subclasses if needed.
    ui_cb_preopt->setVisible(false);
    ui_push_preoptConfig->setVisible(false);
  }

  DefaultEditTab::~DefaultEditTab()
  {
    delete ui;
  }

  void DefaultEditTab::initialize()
  {
    AbstractEditTab::initialize();
  }

}
