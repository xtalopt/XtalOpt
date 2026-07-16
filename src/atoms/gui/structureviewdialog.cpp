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

#include <atoms/gui/structureviewdialog.h>

#include <common/compatibility/platform_defs.h>
#include <common/constants.h>
#include <common/gui/qt_compat_gui.h>
#include <atoms/basis/atom.h>
#include <atoms/eleminfo.h>
#include <atoms/formats/poscarformat.h>
#include <atoms/geometry.h>

#include <QCheckBox>
#include <QCloseEvent>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QDir>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QTextStream>
#include <QVBoxLayout>

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <sstream>

namespace Atoms {
namespace {

// Maximum bond length (current spinbox range).
const double kMaxDisplayableBondLength = 20.0;

// Initial/default bond length (must match widget's constructor default).
const double kDefaultBondLengthMaximum = 2.0;

QString defaultSaveDir()
{
#if GS_WINDOWS
  return QStringLiteral("C:/");
#else
  return QDir::homePath();
#endif
}

QColor speciesPaletteColor(size_t index, size_t speciesCount)
{
  static const QColor palette[] = {
    QColor(55, 126, 184),   // blue
    QColor(228, 26, 28),    // red
    QColor(77, 175, 74),    // green
    QColor(152, 78, 163),   // purple
    QColor(255, 127, 0),    // orange
    QColor(255, 215, 0),    // yellow
    QColor(166, 86, 40),    // brown
    QColor(247, 129, 191),  // pink
    QColor(102, 194, 165),  // teal
    QColor(141, 160, 203),  // lavender
    QColor(231, 138, 195),  // magenta
    QColor(166, 216, 84)    // lime
  };

  const size_t paletteSize = sizeof(palette) / sizeof(palette[0]);
  if (index < paletteSize)
    return palette[index];

  const int hue = static_cast<int>(std::fmod((index * 137.508) + (speciesCount * 19.0), 360.0));
  const int sat = 165 + static_cast<int>((index * 17) % 45);
  const int val = 205 + static_cast<int>((index * 29) % 35);
  return QColor::fromHsv(hue, sat, val);
}

QColor atomColor(const StructureViewSnapshot& snapshot, unsigned short atomicNumber)
{
  std::map<unsigned short, QColor>::const_iterator it = snapshot.speciesColors.find(atomicNumber);
  if (it != snapshot.speciesColors.end())
    return it->second;

  return speciesPaletteColor(snapshot.speciesColors.size() + 1, snapshot.speciesColors.size() + 1);
}

Common::Matrix3 rotationMatrix(double yawDegrees, double pitchDegrees)
{
  const double yaw = yawDegrees * DEG2RAD;
  const double pitch = pitchDegrees * DEG2RAD;

  Common::Matrix3 rotY;
  rotY <<  std::cos(yaw), 0.0, std::sin(yaw),
           0.0,           1.0, 0.0,
          -std::sin(yaw), 0.0, std::cos(yaw);

  Common::Matrix3 rotX;
  rotX << 1.0, 0.0,              0.0,
          0.0, std::cos(pitch), -std::sin(pitch),
          0.0, std::sin(pitch),  std::cos(pitch);

  return rotY * rotX;
}

std::array<Common::Vector3, 8> cellCorners(const StructureViewSnapshot& snapshot)
{
  const Common::Vector3& a = snapshot.aVector;
  const Common::Vector3& b = snapshot.bVector;
  const Common::Vector3& c = snapshot.cVector;
  return { Common::Vector3(0.0, 0.0, 0.0), a, b, c, a + b, a + c, b + c, a + b + c };
}

} // namespace

StructureViewWidget::StructureViewWidget(QWidget* parent)
  : QWidget(parent), m_dragButton(Qt::NoButton), m_panOffset(0.0, 0.0),
    m_yawDegrees(-35.0), m_pitchDegrees(25.0), m_zoomFactor(1.0),
    m_atomLabelsVisible(false), m_bondsVisible(false), m_depthCueingEnabled(false),
    m_bondLengthMinimum(0.0), m_bondLengthMaximum(kDefaultBondLengthMaximum)
{
  setMinimumSize(360, 360);
  setAutoFillBackground(true);
}

void StructureViewWidget::setStructureSnapshot(const StructureViewSnapshot& snapshot)
{
  m_snapshot = snapshot;
  update();
}

void StructureViewWidget::paintEvent(QPaintEvent* event)
{
  Q_UNUSED(event);

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.fillRect(rect(), QColor(250, 250, 250));

  if (m_snapshot.atoms.empty()) {
    painter.setPen(Qt::darkGray);
    painter.drawText(rect(), Qt::AlignCenter, tr("No structure selected"));
    return;
  }

  const Common::Matrix3 rotation = rotationMatrix(m_yawDegrees, m_pitchDegrees);

  std::vector<Common::Vector3> points;
  points.reserve(m_snapshot.atoms.size() + (m_snapshot.hasCell ? 8 : 0));
  for (const auto& atom : m_snapshot.atoms)
    points.push_back(rotation * (atom.position - m_snapshot.center));

  std::array<Common::Vector3, 8> corners;
  if (m_snapshot.hasCell) {
    corners = cellCorners(m_snapshot);
    for (const auto& corner : corners)
      points.push_back(rotation * (corner - m_snapshot.center));
  }

  double maxXY = 1.0;
  for (const auto& point : points) {
    maxXY = std::max(maxXY, std::max(std::fabs(point.x()), std::fabs(point.y())));
  }

  const double pixelScale = 0.42 * std::min(width(), height()) * m_zoomFactor / maxXY;
  const QPointF origin = rect().center() + m_panOffset;

  auto project = [&](const Common::Vector3& point) -> QPointF {
    return origin + QPointF(point.x() * pixelScale, -point.y() * pixelScale);
  };

  if (m_snapshot.hasCell) {
    const std::array<std::pair<int, int>, 12> edges = {{
      {0, 1}, {0, 2}, {0, 3}, {1, 4}, {1, 5}, {2, 4},
      {2, 6}, {3, 5}, {3, 6}, {4, 7}, {5, 7}, {6, 7}
    }};

    painter.setPen(QPen(QColor(140, 140, 140), 1.4));
    std::array<QPointF, 8> projectedCorners;
    for (size_t i = 0; i < corners.size(); ++i)
      projectedCorners[i] = project(rotation * (corners[i] - m_snapshot.center));
    for (const auto& edge : edges)
      painter.drawLine(projectedCorners[edge.first], projectedCorners[edge.second]);

    const QPointF guideOrigin(width() - 72.0, height() - 62.0);
    const double guideLength = 34.0;
    const Common::Vector3 guideA = rotation * m_snapshot.aVector.normalized();
    const Common::Vector3 guideB = rotation * m_snapshot.bVector.normalized();
    const Common::Vector3 guideC = rotation * m_snapshot.cVector.normalized();
    const QPointF guideAEnd =
      guideOrigin + QPointF(guideA.x() * guideLength, -guideA.y() * guideLength);
    const QPointF guideBEnd =
      guideOrigin + QPointF(guideB.x() * guideLength, -guideB.y() * guideLength);
    const QPointF guideCEnd =
      guideOrigin + QPointF(guideC.x() * guideLength, -guideC.y() * guideLength);

    QFont labelFont = painter.font();
    labelFont.setBold(true);
    painter.setFont(labelFont);
    painter.setPen(QPen(QColor(115, 115, 115), 1.4, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(guideOrigin, guideAEnd);
    painter.drawLine(guideOrigin, guideBEnd);
    painter.drawLine(guideOrigin, guideCEnd);
    painter.setBrush(QColor(115, 115, 115));
    painter.drawEllipse(guideOrigin, 2.5, 2.5);
    painter.setPen(QPen(QColor(70, 70, 70)));
    painter.drawText(guideAEnd + QPointF(4.0, -4.0), QStringLiteral("a"));
    painter.drawText(guideBEnd + QPointF(4.0, -4.0), QStringLiteral("b"));
    painter.drawText(guideCEnd + QPointF(4.0, -4.0), QStringLiteral("c"));
  }

  struct ProjectedBond {
    double depth;
    QPointF first;
    QPointF second;
    double alpha;
  };
  std::vector<ProjectedBond> projectedBonds;
  if (m_bondsVisible) {
    projectedBonds.reserve(m_snapshot.bonds.size());
    double minBondDepth = 0.0;
    double maxBondDepth = 0.0;
    bool hasBondDepth = false;
    for (const auto& bond : m_snapshot.bonds) {
      if (bond.first < 0 || bond.second < 0 ||
          static_cast<size_t>(bond.first) >= m_snapshot.atoms.size() ||
          static_cast<size_t>(bond.second) >= m_snapshot.atoms.size() ||
          m_bondLengthMaximum <= 0.0 || bond.length < m_bondLengthMinimum ||
          bond.length > m_bondLengthMaximum) {
        continue;
      }
      const Common::Vector3 p1 = rotation * (m_snapshot.atoms[static_cast<size_t>(bond.first)].position -
                                     m_snapshot.center);
      const Common::Vector3 p2 = rotation * (m_snapshot.atoms[static_cast<size_t>(bond.second)].position -
                                     m_snapshot.center);
      const double depth = 0.5 * (p1.z() + p2.z());
      if (!hasBondDepth) {
        minBondDepth = depth;
        maxBondDepth = depth;
        hasBondDepth = true;
      }
      else {
        minBondDepth = std::min(minBondDepth, depth);
        maxBondDepth = std::max(maxBondDepth, depth);
      }
      projectedBonds.push_back({depth, project(p1), project(p2), 1.0});
    }
    if (m_depthCueingEnabled && maxBondDepth > minBondDepth) {
      const double depthSpan = maxBondDepth - minBondDepth;
      for (auto& bond : projectedBonds)
        bond.alpha = 0.38 + 0.62 * ((bond.depth - minBondDepth) / depthSpan);
    }
  }
  std::sort(projectedBonds.begin(), projectedBonds.end(),
            [](const ProjectedBond& a, const ProjectedBond& b) {
              return a.depth < b.depth;
            });
  for (const auto& bond : projectedBonds) {
    QColor bondColor(125, 125, 125);
    bondColor.setAlphaF(bond.alpha);
    painter.setPen(QPen(bondColor, 2.0, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(bond.first, bond.second);
  }

  struct ProjectedAtom {
    double depth;
    double radius;
    QColor color;
    QPointF position;
    QString label;
    double alpha;
  };

  std::vector<ProjectedAtom> projectedAtoms;
  projectedAtoms.reserve(m_snapshot.atoms.size());
  double minAtomDepth = 0.0;
  double maxAtomDepth = 0.0;
  bool hasAtomDepth = false;
  for (const auto& atom : m_snapshot.atoms) {
    const Common::Vector3 rotated = rotation * (atom.position - m_snapshot.center);
    const double radius = std::max(4.0, std::min(18.0, atom.covalentRadius * pixelScale * 0.30));
    if (!hasAtomDepth) {
      minAtomDepth = rotated.z();
      maxAtomDepth = rotated.z();
      hasAtomDepth = true;
    }
    else {
      minAtomDepth = std::min(minAtomDepth, rotated.z());
      maxAtomDepth = std::max(maxAtomDepth, rotated.z());
    }
    projectedAtoms.push_back({
      rotated.z(),
      radius,
      atomColor(m_snapshot, atom.atomicNumber),
      project(rotated),
      QString::fromStdString(Atoms::ElementInfo::getAtomicSymbol(atom.atomicNumber)),
      1.0
    });
  }

  if (m_depthCueingEnabled && maxAtomDepth > minAtomDepth) {
    const double depthSpan = maxAtomDepth - minAtomDepth;
    for (auto& atom : projectedAtoms)
      atom.alpha = 0.45 + 0.55 * ((atom.depth - minAtomDepth) / depthSpan);
  }

  std::sort(projectedAtoms.begin(), projectedAtoms.end(),
            [](const ProjectedAtom& a, const ProjectedAtom& b) {
              return a.depth < b.depth;
            });

  for (const auto& atom : projectedAtoms) {
    QColor atomColor = atom.color;
    atomColor.setAlphaF(atom.alpha);
    QRadialGradient gradient(atom.position - QPointF(atom.radius * 0.35, atom.radius * 0.35),
                             atom.radius * 1.2);
    gradient.setColorAt(0.0, atomColor.lighter(150));
    gradient.setColorAt(1.0, atomColor.darker(130));
    painter.setBrush(gradient);
    painter.setPen(QPen(atomColor.darker(160), 1.0));
    painter.drawEllipse(atom.position, atom.radius, atom.radius);
  }

  if (m_atomLabelsVisible) {
    QFont labelFont = painter.font();
    labelFont.setBold(true);
    painter.setFont(labelFont);
    for (const auto& atom : projectedAtoms) {
      painter.setPen(QPen(QColor(255, 255, 255, 210), 3.0));
      painter.drawText(atom.position + QPointF(atom.radius * 0.45, -atom.radius * 0.45),
                       atom.label);
      painter.setPen(QPen(QColor(35, 35, 35)));
      painter.drawText(atom.position + QPointF(atom.radius * 0.45, -atom.radius * 0.45),
                       atom.label);
    }
  }

  std::map<unsigned short, int> atomCounts;
  for (const auto& atom : m_snapshot.atoms)
    atomCounts[atom.atomicNumber] += 1;

  if (!atomCounts.empty()) {
    QFont legendFont = painter.font();
    legendFont.setBold(false);
    painter.setFont(legendFont);
    const QFontMetrics fm(legendFont);
    const int rowHeight = std::max(18, fm.height() + 4);
    int legendWidth = 0;
    for (std::map<unsigned short, int>::const_iterator it = atomCounts.begin();
         it != atomCounts.end(); ++it) {
      const QString symbol = QString::fromStdString(Atoms::ElementInfo::getAtomicSymbol(it->first));
      legendWidth = std::max(legendWidth, fm.boundingRect(symbol).width());
    }
    legendWidth += 34;

    const QRect legendRect(10, 10, legendWidth,
                           12 + rowHeight * static_cast<int>(atomCounts.size()));
    painter.setPen(QPen(QColor(205, 205, 205)));
    painter.setBrush(QColor(255, 255, 255, 225));
    painter.drawRoundedRect(legendRect, 4, 4);

    int y = legendRect.top() + 8;
    for (std::map<unsigned short, int>::const_iterator it = atomCounts.begin();
         it != atomCounts.end(); ++it) {
      const QString symbol = QString::fromStdString(Atoms::ElementInfo::getAtomicSymbol(it->first));
      const QColor color = atomColor(m_snapshot, it->first);
      const QRect swatch(legendRect.left() + 8, y + 3, 10, 10);
      painter.setPen(QPen(color.darker(150)));
      painter.setBrush(color);
      painter.drawEllipse(swatch);
      painter.setPen(QPen(QColor(45, 45, 45)));
      painter.drawText(legendRect.left() + 26, y + fm.ascent() + 2, symbol);
      y += rowHeight;
    }
  }
}

void StructureViewWidget::resetView()
{
  m_panOffset = QPointF(0.0, 0.0);
  m_yawDegrees = -35.0;
  m_pitchDegrees = 25.0;
  m_zoomFactor = 1.0;
  update();
}

void StructureViewWidget::setAtomLabelsVisible(bool visible)
{
  m_atomLabelsVisible = visible;
  update();
}

void StructureViewWidget::setBondsVisible(bool visible)
{
  m_bondsVisible = visible;
  update();
}

void StructureViewWidget::setDepthCueingEnabled(bool enabled)
{
  m_depthCueingEnabled = enabled;
  update();
}

void StructureViewWidget::setBondLengthMinimum(double minimum)
{
  m_bondLengthMinimum = minimum;
  update();
}

void StructureViewWidget::setBondLengthMaximum(double maximum)
{
  m_bondLengthMaximum = maximum;
  update();
}

void StructureViewWidget::mousePressEvent(QMouseEvent* event)
{
  m_dragButton = event->button();
  m_lastMousePos = QtCompat::mouseEventPos(*event);
}

void StructureViewWidget::mouseMoveEvent(QMouseEvent* event)
{
  if (m_dragButton == Qt::NoButton)
    return;

  const QPoint pos = QtCompat::mouseEventPos(*event);
  const QPoint delta = pos - m_lastMousePos;
  m_lastMousePos = pos;

  if (m_dragButton == Qt::LeftButton) {
    m_yawDegrees += delta.x() * 0.6;
    m_pitchDegrees += delta.y() * 0.6;
    m_pitchDegrees = std::max(-89.0, std::min(89.0, m_pitchDegrees));
  }
  else if (m_dragButton == Qt::RightButton) {
    m_panOffset += delta;
  }

  update();
}

void StructureViewWidget::mouseReleaseEvent(QMouseEvent* event)
{
  Q_UNUSED(event);
  m_dragButton = Qt::NoButton;
}

void StructureViewWidget::wheelEvent(QWheelEvent* event)
{
  const QPoint delta = event->angleDelta();
  if (delta.y() == 0) {
    event->ignore();
    return;
  }

  const double factor = (delta.y() > 0) ? 1.10 : 1.0 / 1.10;
  m_zoomFactor = std::max(0.20, std::min(8.0, m_zoomFactor * factor));
  update();
  event->accept();
}

StructureViewDialog::StructureViewDialog(QWidget* parent)
  : QDialog(parent), m_infoLabel(new QLabel(this)),
    m_viewWidget(new StructureViewWidget(this)),
    m_optionsWidget(new QWidget(this)),
    m_resetViewButton(new QPushButton(tr("Reset View"), this)),
    m_saveDataButton(new QPushButton(tr("Save Data"), this)),
    m_saveImageButton(new QPushButton(tr("Save Image"), this)),
    m_closeButton(new QPushButton(tr("Close"), this))
{
  setWindowTitle(tr("Structure Viewer"));
  setAttribute(Qt::WA_DeleteOnClose, false);
  resize(520, 560);

  auto* layout = new QVBoxLayout(this);
  m_infoLabel->setWordWrap(true);
  layout->addWidget(m_infoLabel);
  layout->addWidget(m_viewWidget, 1);

  auto* atomLabelsBox = new QCheckBox(tr("Atom labels"), m_optionsWidget);
  auto* depthCueBox = new QCheckBox(tr("Depth cueing"), m_optionsWidget);
  auto* bondsBox = new QCheckBox(tr("Bonds"), m_optionsWidget);
  auto* bondMinSpin = new QDoubleSpinBox(m_optionsWidget);
  auto* bondMaxSpin = new QDoubleSpinBox(m_optionsWidget);
  atomLabelsBox->setChecked(false);
  depthCueBox->setChecked(false);
  bondsBox->setChecked(false);
  bondMinSpin->setRange(0.0, kMaxDisplayableBondLength);
  bondMinSpin->setDecimals(3);
  bondMinSpin->setSingleStep(0.005);
  bondMinSpin->setSuffix(tr(" A"));
  bondMinSpin->setValue(0.0);
  bondMaxSpin->setRange(0.0, kMaxDisplayableBondLength);
  bondMaxSpin->setDecimals(3);
  bondMaxSpin->setSingleStep(0.005);
  bondMaxSpin->setSuffix(tr(" A"));
  bondMaxSpin->setValue(kDefaultBondLengthMaximum);
  atomLabelsBox->setFocusPolicy(Qt::StrongFocus);
  depthCueBox->setFocusPolicy(Qt::StrongFocus);
  bondsBox->setFocusPolicy(Qt::StrongFocus);
  bondMinSpin->setFocusPolicy(Qt::StrongFocus);
  bondMaxSpin->setFocusPolicy(Qt::StrongFocus);
  m_resetViewButton->setAutoDefault(false);
  m_resetViewButton->setDefault(false);
  m_saveDataButton->setAutoDefault(false);
  m_saveDataButton->setDefault(false);
  m_saveImageButton->setAutoDefault(false);
  m_saveImageButton->setDefault(false);
  m_closeButton->setAutoDefault(false);
  m_closeButton->setDefault(false);
  m_resetViewButton->setFocusPolicy(Qt::StrongFocus);
  m_saveDataButton->setFocusPolicy(Qt::StrongFocus);
  m_saveImageButton->setFocusPolicy(Qt::StrongFocus);
  m_closeButton->setFocusPolicy(Qt::StrongFocus);

  auto* optionsLayout = new QHBoxLayout(m_optionsWidget);
  optionsLayout->setContentsMargins(0, 0, 0, 0);
  optionsLayout->addWidget(atomLabelsBox);
  optionsLayout->addWidget(depthCueBox);
  optionsLayout->addWidget(bondsBox);
  optionsLayout->addWidget(bondMinSpin);
  optionsLayout->addWidget(new QLabel(tr("to"), m_optionsWidget));
  optionsLayout->addWidget(bondMaxSpin);
  optionsLayout->addStretch(1);
  layout->addWidget(m_optionsWidget);

  auto* buttonLayout = new QHBoxLayout;
  buttonLayout->addStretch(1);
  buttonLayout->addWidget(m_resetViewButton);
  buttonLayout->addWidget(m_saveDataButton);
  buttonLayout->addWidget(m_saveImageButton);
  buttonLayout->addWidget(m_closeButton);
  layout->addLayout(buttonLayout);

  setTabOrder(atomLabelsBox, depthCueBox);
  setTabOrder(depthCueBox, bondsBox);
  setTabOrder(bondsBox, bondMinSpin);
  setTabOrder(bondMinSpin, bondMaxSpin);
  setTabOrder(bondMaxSpin, m_resetViewButton);
  setTabOrder(m_resetViewButton, m_saveDataButton);
  setTabOrder(m_saveDataButton, m_saveImageButton);
  setTabOrder(m_saveImageButton, m_closeButton);

  connect(m_resetViewButton, &QPushButton::clicked, m_viewWidget, &StructureViewWidget::resetView);
  connect(atomLabelsBox, &QCheckBox::toggled,
          m_viewWidget, &StructureViewWidget::setAtomLabelsVisible);
  connect(depthCueBox, &QCheckBox::toggled,
          m_viewWidget, &StructureViewWidget::setDepthCueingEnabled);
  connect(bondsBox, &QCheckBox::toggled, m_viewWidget, &StructureViewWidget::setBondsVisible);
  connect(bondMinSpin,
          static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
          m_viewWidget, &StructureViewWidget::setBondLengthMinimum);
  connect(bondMaxSpin,
          static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
          m_viewWidget, &StructureViewWidget::setBondLengthMaximum);
  connect(bondMinSpin,
          static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
          this, [bondMaxSpin](double minimum) {
            bondMaxSpin->setMinimum(minimum);
            if (bondMaxSpin->value() < minimum)
              bondMaxSpin->setValue(minimum);
          });
  connect(m_saveDataButton, &QPushButton::clicked, this, &StructureViewDialog::saveData);
  connect(m_saveImageButton, &QPushButton::clicked, this, &StructureViewDialog::saveImage);
  connect(m_closeButton, &QPushButton::clicked, this, &StructureViewDialog::close);
}

void StructureViewDialog::displayStructure(const Geometry& structure, const QString& label)
{
  StructureViewSnapshot snapshot;
  snapshot.label = label;
  snapshot.formula = structure.getChemicalFormula();
  snapshot.hasCell = structure.is3D();
  if (snapshot.hasCell) {
    snapshot.aVector = structure.unitCell().aVector();
    snapshot.bVector = structure.unitCell().bVector();
    snapshot.cVector = structure.unitCell().cVector();
    snapshot.center = 0.5 * (snapshot.aVector + snapshot.bVector + snapshot.cVector);
  }

  snapshot.atoms.reserve(structure.numAtoms());
  Common::Vector3 centroid = Common::Vector3::Zero();
  for (size_t i = 0; i < structure.numAtoms(); ++i) {
    const Atom& atom = structure.atom(i);
    const unsigned short atomicNumber = atom.atomicNumber();
    snapshot.atoms.push_back(
      {atomicNumber, atom.pos(), ElementInfo::getCovalentRadius(atomicNumber)});
    centroid += atom.pos();
  }

  std::map<unsigned short, int> species;
  for (const auto& atom : snapshot.atoms)
    species[atom.atomicNumber] += 1;

  size_t colorIndex = 0;
  for (std::map<unsigned short, int>::const_iterator it = species.begin();
       it != species.end(); ++it) {
    snapshot.speciesColors[it->first] = speciesPaletteColor(colorIndex, species.size());
    ++colorIndex;
  }

  if (!snapshot.hasCell && !snapshot.atoms.empty())
    snapshot.center = centroid / static_cast<double>(snapshot.atoms.size());

  for (size_t first = 0; first < snapshot.atoms.size(); ++first) {
    for (size_t second = first + 1; second < snapshot.atoms.size(); ++second) {
      const double length =
        (snapshot.atoms[first].position - snapshot.atoms[second].position).norm();
      if (length <= kMaxDisplayableBondLength) {
        snapshot.bonds.push_back({static_cast<int>(first), static_cast<int>(second), length});
      }
    }
  }

  std::ostringstream poscarStream;
  if (snapshot.hasCell && PoscarFormat::write(structure, poscarStream, label))
    m_poscarText = QString::fromStdString(poscarStream.str());
  else
    m_poscarText.clear();

  m_viewWidget->setStructureSnapshot(snapshot);
  m_infoLabel->setText(
    tr("%1%2%3")
      .arg(snapshot.label.isEmpty() ? QString() : snapshot.label)
      .arg(snapshot.label.isEmpty() || snapshot.formula.isEmpty() ? QString() : "  -  ")
      .arg(snapshot.formula));
  setWindowTitle(snapshot.label.isEmpty() ? tr("Structure Viewer")
                   : tr("Structure Viewer - %1").arg(snapshot.label));

  if (!isVisible())
    show();
  raise();
  activateWindow();
}

void StructureViewDialog::saveImage() const
{
  const QString defaultName = m_viewWidget->structureSnapshot().label.isEmpty()
                                ? QStringLiteral("structure-view.png")
                                : m_viewWidget->structureSnapshot().label +
                                    QStringLiteral("-structure-view.png");
  QString filename = QFileDialog::getSaveFileName(
    const_cast<StructureViewDialog*>(this), tr("Save Structure Image"),
    QDir(defaultSaveDir()).filePath(defaultName),
    tr("PNG Image (*.png);;JPEG Image (*.jpg);;BMP Image (*.bmp)"),
    nullptr, QFileDialog::DontUseNativeDialog);
  if (filename.isEmpty())
    return;

  const int exportScale = 2;
  QImage image(m_viewWidget->size() * exportScale, QImage::Format_ARGB32_Premultiplied);
  image.fill(Qt::transparent);
  QPainter painter(&image);
  painter.scale(exportScale, exportScale);
  m_viewWidget->render(&painter, QPoint(), QRegion(), QWidget::DrawChildren);
  image.save(filename);
}

void StructureViewDialog::saveData() const
{
  if (m_poscarText.isEmpty()) {
    QMessageBox::warning(const_cast<StructureViewDialog*>(this), tr("Save Structure Data"),
                         tr("POSCAR data is not available for this structure."));
    return;
  }

  const QString defaultName = m_viewWidget->structureSnapshot().label.isEmpty()
                                ? QStringLiteral("POSCAR")
                                : m_viewWidget->structureSnapshot().label + QStringLiteral(".vasp");
  QString filename = QFileDialog::getSaveFileName(
    const_cast<StructureViewDialog*>(this), tr("Save Structure Data"),
    QDir(defaultSaveDir()).filePath(defaultName), tr("VASP POSCAR (*.vasp);;All files (*.*)"),
    nullptr, QFileDialog::DontUseNativeDialog);
  if (filename.isEmpty())
    return;

  QFile file(filename);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QMessageBox::warning(const_cast<StructureViewDialog*>(this), tr("Save Structure Data"),
                         tr("Could not open '%1' for writing.").arg(filename));
    return;
  }

  QTextStream stream(&file);
  stream << m_poscarText;
}

} // namespace Atoms
