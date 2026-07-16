/**********************************************************************
  qt_compat_gui - GUI-only compatibility shims (Qt::Gui / Qt::Widgets).

  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef COMMON_QT_COMPAT_GUI_H
#define COMMON_QT_COMPAT_GUI_H

#include <QtGlobal>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QPoint>
#include <QRect>
#include <QScreen>

// GUI Qt compatibility helpers.
namespace QtCompat {

// Return the primary screen geometry.
inline QRect primaryScreenAvailableGeometry()
{
  if (QScreen* scr = QGuiApplication::primaryScreen())
    return scr->availableGeometry();
  return QRect(0, 0, 1024, 768);
}

// Return the local mouse position.
inline QPoint mouseEventPos(const QMouseEvent& event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  return event.position().toPoint();
#else
  return event.localPos().toPoint();
#endif
}

} // namespace QtCompat

#endif // COMMON_QT_COMPAT_GUI_H
