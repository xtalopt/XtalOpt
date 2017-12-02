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

#include <examplesearch/ui/tab_init.h>

#include <examplesearch/examplesearch.h>
#include <examplesearch/ui/dialog.h>

#include <globalsearch/macros.h>

#include <avogadro/moleculefile.h>

#include <QSettings>

#include <QFileDialog>
#include <QMessageBox>

using namespace std;
using namespace Avogadro;

namespace ExampleSearch {

TabInit::TabInit(ExampleSearchDialog* dialog, ExampleSearch* opt)
  : AbstractTab(dialog, opt)
{
  ui.setupUi(m_tab_widget);

  initialize();
}

TabInit::~TabInit()
{
}

void TabInit::lockGUI()
{
}

void TabInit::updateParams()
{
}
}
