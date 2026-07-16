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

class QCloseEvent;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QSpinBox;

namespace GenerateXrd {

class XrdPlot;

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
  bool m_hasStructure;
  Atoms::Geometry m_geometry;
  QString m_structureLabel;
  QLabel* m_infoLabel;
  XrdPlot* m_plot;
  QDoubleSpinBox* m_wavelengthSpin;
  QDoubleSpinBox* m_peakwidthSpin;
  QSpinBox* m_numpointsSpin;
  QDoubleSpinBox* m_max2thetaSpin;
  QPushButton* m_resetParametersButton;
  QPushButton* m_resetViewButton;
  QPushButton* m_saveDataButton;
  QPushButton* m_saveImageButton;
  QPushButton* m_closeButton;
  XrdData m_lastData;
  QString m_lastTag;
  double m_lastWavelength;
  double m_lastPeakwidth;
  size_t m_lastNumpoints;
  double m_lastMax2theta;
};

} // namespace GenerateXrd

#endif // GLOBALXRD_XRDVIEWDIALOG_H
