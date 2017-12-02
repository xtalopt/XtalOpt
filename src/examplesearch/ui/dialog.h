/**********************************************************************
  ExampleSearch -- A tool for analysing a matrix-substrate docking problem

  Copyright (C) 2012 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef EXAMPLESEARCHDIALOG_H
#define EXAMPLESEARCHDIALOG_H

#include <examplesearch/examplesearch.h>

#include <globalsearch/ui/abstractdialog.h>

#include "ui_dialog.h"

namespace Avogadro {
class Molecule;
class GLWidget;
}

namespace ExampleSearch {
class TabInit;
class TabEdit;
class TabProgress;
class TabPlot;
class TabLog;
class ExampleSearch;

class ExampleSearchDialog : public GlobalSearch::AbstractDialog
{
  Q_OBJECT

public:
  explicit ExampleSearchDialog(Avogadro::GLWidget* glWidget = 0,
                               QWidget* parent = 0, Qt::WindowFlags f = 0);
  virtual ~ExampleSearchDialog();

public slots:
  void saveSession();

private slots:
  void startSearch();

signals:

private:
  Ui::ExampleSearchDialog ui;

  TabInit* m_tab_init;
  TabEdit* m_tab_edit;
  TabProgress* m_tab_progress;
  TabPlot* m_tab_plot;
  TabLog* m_tab_log;
};
}

#endif
