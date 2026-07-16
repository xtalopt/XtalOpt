/**********************************************************************
  StructureViewDialog - Reusable structure viewer.

  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef ATOMS_STRUCTUREVIEWDIALOG_H
#define ATOMS_STRUCTUREVIEWDIALOG_H

#include <common/matrix.h>
#include <common/vector.h>

#include <QColor>
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QString>
#include <QWidget>

#include <map>
#include <vector>

class QCloseEvent;

namespace Atoms {

class Geometry;

struct StructureViewAtom
{
  unsigned short atomicNumber;
  Common::Vector3 position;
  double covalentRadius;
};

struct StructureViewBond
{
  int first;
  int second;
  double length;
};

struct StructureViewSnapshot
{
  QString label;
  QString formula;
  // Default values for an empty structure.
  bool hasCell = false;
  std::vector<StructureViewAtom> atoms;
  std::vector<StructureViewBond> bonds;
  std::map<unsigned short, QColor> speciesColors;
  Common::Vector3 center = Common::Vector3::Zero();
  Common::Vector3 aVector = Common::Vector3::Zero();
  Common::Vector3 bVector = Common::Vector3::Zero();
  Common::Vector3 cVector = Common::Vector3::Zero();
};

// Widget for drawing a structure (cell and atoms).
class StructureViewWidget : public QWidget
{
  Q_OBJECT

public:
  explicit StructureViewWidget(QWidget* parent = nullptr);

  void setStructureSnapshot(const StructureViewSnapshot& snapshot);
  const StructureViewSnapshot& structureSnapshot() const { return m_snapshot; }

public slots:
  void resetView();
  void setAtomLabelsVisible(bool visible);
  void setBondsVisible(bool visible);
  void setDepthCueingEnabled(bool enabled);
  void setBondLengthMinimum(double minimum);
  void setBondLengthMaximum(double maximum);

protected:
  void paintEvent(QPaintEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;

private:
  StructureViewSnapshot m_snapshot;
  QPoint m_lastMousePos;
  Qt::MouseButton m_dragButton;
  QPointF m_panOffset;
  double m_yawDegrees;
  double m_pitchDegrees;
  double m_zoomFactor;
  bool m_atomLabelsVisible;
  bool m_bondsVisible;
  bool m_depthCueingEnabled;
  double m_bondLengthMinimum;
  double m_bondLengthMaximum;
};

// Dialog for viewing a structure with various options and controls.
class StructureViewDialog : public QDialog
{
  Q_OBJECT

public:
  explicit StructureViewDialog(QWidget* parent = nullptr);
  void displayStructure(const Atoms::Geometry& structure, const QString& label = QString());

public slots:
  void saveImage() const;
  void saveData() const;

private:
  QLabel* m_infoLabel;
  StructureViewWidget* m_viewWidget;
  QWidget* m_optionsWidget;
  QPushButton* m_resetViewButton;
  QPushButton* m_saveDataButton;
  QPushButton* m_saveImageButton;
  QPushButton* m_closeButton;
  QString m_poscarText;
};

} // namespace Atoms

#endif
