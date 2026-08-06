/**********************************************************************
  TabAbout - The About tab: program version and credits.

  Copyright (C) 2025 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <xtalopt/gui/tab_about.h>

#include <xtalopt/gui/dialog.h>
#include <xtalopt/xtalopt.h>

#include <QDateTime>
#include <QLabel>
#include <QGraphicsOpacityEffect>
#include <QPixmap>
#include <QPainter>

using namespace std;

namespace XtalOpt {

TabAbout::TabAbout(Search::AbstractDialog* parent, XtalOpt* p)
  : AbstractTab(parent, p)
{
  ui.setupUi(m_tab_widget);

  // For the "About" tab, we will have three separate "QLable" objects:
  // 1) The code logo,
  // 2) The title (code name, version, brief description),
  // 3) The details (group, websites, etc).

  // Insert the logo
  QPixmap pixmap(":/xtalopt/icons/images/xtalopt-logo.png");
  if (!pixmap.isNull()) {
    QPixmap resizedPixmap = pixmap.scaled(100, 100, Qt::IgnoreAspectRatio, Qt::FastTransformation);
    ui.about_logo->setPixmap(resizedPixmap);
  }

  // Insert the title
  QString labelText;
  labelText.append("<p><b><font size=\"3\">");
  labelText.append(QString("XtalOpt  (version %1)").arg(XTALOPT_VER));
  labelText.append("</font></b></p>");
  labelText.append("<p><i>A multi-objective evolutionary algorithm for variable-composition ground state search.</i></p>");
  labelText.append(QString("<p>Built with: Qt-%1, Qwt-%2").arg(QT_VER).arg(QWT_VER));
  labelText.append(QString(", SSH-%1").arg(SSH_VER));
  labelText.append(QString("</p>"));
  ui.about_title->setText(labelText);

  // The detail information are set in "ui" file!

  initialize();
}

TabAbout::~TabAbout()
{
}

void TabAbout::disconnectGUI()
{
  disconnect(m_dialog, 0, this, 0);
}

}
