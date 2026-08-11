/**********************************************************************
  XrdViewDialog - Reusable simulated Xrd viewer.

  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef GLOBALXRD_XRDVIEWDIALOG_H
#define GLOBALXRD_XRDVIEWDIALOG_H

#include <generatexrd/generatexrd.h>

#include <atoms/geometry.h>

#include <QDialog>
#include <QString>

#include <memory>

class QCloseEvent;

// The generated design header is included by the source file only: this header
//   is also used from other modules, which do not build this design.
namespace Ui {
class XrdViewDialog;
}

namespace GenerateXrd {

// Window for an XRD pattern.
class XrdViewDialog : public QDialog
{
  Q_OBJECT

public:
  explicit XrdViewDialog(QWidget* parent = nullptr);
  virtual ~XrdViewDialog() override;

public slots:
  void displayStructure(const Atoms::Geometry& structure, const QString& label = QString());
  void resetParameters();
  void resetView();
  void updatePlot();
  void saveImage() const;
  void saveData() const;

private:
  std::unique_ptr<Ui::XrdViewDialog> ui;
  bool m_hasStructure;
  Atoms::Geometry m_geometry;
  QString m_structureLabel;
  XrdData m_lastData;
  QString m_lastTag;
  double m_lastWavelength;
  double m_lastPeakwidth;
  size_t m_lastNumpoints;
  double m_lastMax2theta;
};

} // namespace GenerateXrd

#endif // GLOBALXRD_XRDVIEWDIALOG_H
