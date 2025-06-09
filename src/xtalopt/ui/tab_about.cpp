/**********************************************************************
  XtalOpt - Tools for advanced crystal optimization

  Copyright (C) 2025 by Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <xtalopt/ui/tab_about.h>

#include <xtalopt/ui/dialog.h>
#include <xtalopt/xtalopt.h>

#include <QDateTime>
#include <QSettings>
#include <QLabel>
#include <QGraphicsOpacityEffect>
#include <QPixmap>
#include <QPainter>

using namespace std;

namespace XtalOpt {

TabAbout::TabAbout(GlobalSearch::AbstractDialog* parent, XtalOpt* p)
  : AbstractTab(parent, p)
{
  ui.setupUi(m_tab_widget);

  // For the "About" tab, we will have three separate "QLable" objects:
  // 1) The code logo,
  // 2) The title (code name, version, brief description),
  // 3) The details (group, websites, etc).

  // Insert the logo
  QPixmap pixmap(":/xtalopt/icons/images/xtalopt-logo.png");
  QPixmap resizedPixmap = pixmap.scaled(100, 100, Qt::IgnoreAspectRatio, Qt::FastTransformation);
  ui.about_logo->setPixmap(resizedPixmap);

  // Insert the title
  QString labelText;
  labelText.append("<P><b><font_size=5>");
  labelText.append(QString("XtalOpt  (version %1)").arg(XTALOPT_VER));
  labelText.append("</font></b></P>");
  labelText.append("<P><i>A multi-objective evolutionary algorithm for variable-composition ground state search.</i></P></br>");
  ui.about_title->setText(labelText);

  // The detail information are set in "ui" file!

  initialize();
}

TabAbout::~TabAbout()
{
}

void TabAbout::disconnectGUI()
{
  connect(m_dialog, 0, this, 0);
}

}
