/**********************************************************************
  DefaultOptTab - Simple implementation of AbstractOptTab

  Copyright (C) 2011 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <search/gui/defaultopttab.h>

#include "ui_defaultopttab.h"

#include <QAction>
#include <QIcon>
#include <QLineEdit>
#include <QStyle>

namespace Search {

DefaultOptTab::DefaultOptTab(AbstractDialog* dialog, SearchBase* opt)
  : AbstractOptTab(dialog, opt), ui(new Ui::DefaultOptTab)
{
  ui->setupUi(m_tab_widget);

  ui_action_browse_locpath =
    new QAction(ui->edit_locpath->style()->standardIcon(QStyle::SP_DirOpenIcon),
                tr("Browse..."), ui->edit_locpath);
  ui_action_browse_locpath->setToolTip(tr("Select an absolute local working directory."));
  ui->edit_locpath->addAction(ui_action_browse_locpath, QLineEdit::TrailingPosition);

  ui_combo_queueInterfaces = ui->combo_queueInterfaces;
  ui_combo_optimizers = ui->combo_optimizers;
  ui_combo_templates = ui->combo_templates;
  ui_edit_user1 = ui->edit_user1;
  ui_edit_user2 = ui->edit_user2;
  ui_edit_user3 = ui->edit_user3;
  ui_edit_user4 = ui->edit_user4;
  ui_list_optStep = ui->list_optStep;
  ui_push_add = ui->push_add;
  ui_push_help = ui->push_help;
  ui_push_loadScheme = ui->push_loadScheme;
  ui_push_optimizerConfig = ui->push_optimizerConfig;
  ui_push_queueInterfaceConfig = ui->push_queueInterfaceConfig;
  ui_push_remove = ui->push_remove;
  ui_push_saveScheme = ui->push_saveScheme;
  ui_edit_opt = ui->edit_edit;
  ui_edit_locpath = ui->edit_locpath;
  ui_edit_description = ui->edit_description;
  ui_cb_logErrorDirs = ui->cb_logErrorDirs;
}

DefaultOptTab::~DefaultOptTab()
{
  delete ui;
}

void DefaultOptTab::initialize()
{
  AbstractOptTab::initialize();
}
}
