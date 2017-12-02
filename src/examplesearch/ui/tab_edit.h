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

#ifndef TAB_EDIT_H
#define TAB_EDIT_H

#include <globalsearch/ui/defaultedittab.h>

namespace ExampleSearch {
class ExampleSearch;
class ExampleSearchDialog;

class TabEdit : public GlobalSearch::DefaultEditTab
{
  Q_OBJECT

public:
  explicit TabEdit(ExampleSearchDialog* parent, ExampleSearch* p);
  virtual ~TabEdit();

public slots:
  void readSettings(const QString& filename = "");
  void writeSettings(const QString& filename = "");
};
}

#endif
