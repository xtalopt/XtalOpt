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

#include <generatexrd/gui/xrdviewdialog.h>

#include <common/compatibility/platform_defs.h>
#include <generatexrd/gui/xrd_plot.h>
#include <common/output.h>

#include <QCloseEvent>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QSizePolicy>
#include <QSpinBox>
#include <QTextStream>
#include <QVBoxLayout>

namespace GenerateXrd {
namespace {

QString defaultSaveDir()
{
#if GS_WINDOWS
  return QStringLiteral("C:/");
#else
  return QDir::homePath();
#endif
}

} // namespace

XrdViewDialog::XrdViewDialog(QWidget* parent)
  : QDialog(parent),
    m_hasStructure(false),
    m_infoLabel(new QLabel(this)),
    m_plot(new XrdPlot(this)),
    m_wavelengthSpin(new QDoubleSpinBox(this)),
    m_peakwidthSpin(new QDoubleSpinBox(this)),
    m_numpointsSpin(new QSpinBox(this)),
    m_max2thetaSpin(new QDoubleSpinBox(this)),
    m_resetParametersButton(new QPushButton(tr("Reset Parameters"), this)),
    m_resetViewButton(new QPushButton(tr("Reset View"), this)),
    m_saveDataButton(new QPushButton(tr("Save Data"), this)),
    m_saveImageButton(new QPushButton(tr("Save Image"), this)),
    m_closeButton(new QPushButton(tr("Close"), this)),
    m_lastWavelength(DEFAULT_WAVELENGTH),
    m_lastPeakwidth(DEFAULT_PEAKWIDTH),
    m_lastNumpoints(DEFAULT_NUMPOINTS),
    m_lastMax2theta(DEFAULT_MAX_2THETA)
{
  setWindowTitle(tr("Simulated XRD Pattern"));
  resize(640, 640);

  auto* layout = new QVBoxLayout(this);
  m_infoLabel->setWordWrap(true);
  layout->addWidget(m_infoLabel);
  m_plot->setMinimumSize(480, 480);
  m_plot->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  layout->addWidget(m_plot, 1);

  m_wavelengthSpin->setRange(0.01, 100.0);
  m_wavelengthSpin->setDecimals(5);
  m_wavelengthSpin->setSingleStep(0.01);
  m_wavelengthSpin->setValue(m_lastWavelength);
  m_peakwidthSpin->setRange(0.0001, 100.0);
  m_peakwidthSpin->setDecimals(5);
  m_peakwidthSpin->setSingleStep(0.01);
  m_peakwidthSpin->setValue(m_lastPeakwidth);
  m_numpointsSpin->setRange(10, 1000000);
  m_numpointsSpin->setSingleStep(100);
  m_numpointsSpin->setValue(static_cast<int>(m_lastNumpoints));
  m_max2thetaSpin->setRange(1.0, 360.0);
  m_max2thetaSpin->setDecimals(3);
  m_max2thetaSpin->setSingleStep(1.0);
  m_max2thetaSpin->setValue(m_lastMax2theta);

  auto* options = new QHBoxLayout;
  options->addWidget(new QLabel(tr("Wavelength"), this));
  options->addWidget(m_wavelengthSpin);
  options->addWidget(new QLabel(tr("Peak width"), this));
  options->addWidget(m_peakwidthSpin);
  options->addWidget(new QLabel(tr("Points"), this));
  options->addWidget(m_numpointsSpin);
  options->addWidget(new QLabel(tr("Max 2theta"), this));
  options->addWidget(m_max2thetaSpin);
  options->addStretch(1);
  layout->addLayout(options);

  auto* buttons = new QHBoxLayout;
  buttons->addWidget(m_resetParametersButton);
  buttons->addStretch(1);
  buttons->addWidget(m_resetViewButton);
  buttons->addWidget(m_saveDataButton);
  buttons->addWidget(m_saveImageButton);
  buttons->addWidget(m_closeButton);
  layout->addLayout(buttons);

  m_saveDataButton->setEnabled(false);
  m_saveImageButton->setEnabled(false);
  m_resetParametersButton->setAutoDefault(false);
  m_resetParametersButton->setDefault(false);
  m_resetViewButton->setAutoDefault(false);
  m_resetViewButton->setDefault(false);
  m_saveDataButton->setAutoDefault(false);
  m_saveDataButton->setDefault(false);
  m_saveImageButton->setAutoDefault(false);
  m_saveImageButton->setDefault(false);
  m_closeButton->setAutoDefault(false);
  m_closeButton->setDefault(false);
  m_resetParametersButton->setFocusPolicy(Qt::StrongFocus);
  m_resetViewButton->setFocusPolicy(Qt::StrongFocus);
  m_saveDataButton->setFocusPolicy(Qt::StrongFocus);
  m_saveImageButton->setFocusPolicy(Qt::StrongFocus);
  m_closeButton->setFocusPolicy(Qt::StrongFocus);
  setTabOrder(m_wavelengthSpin, m_peakwidthSpin);
  setTabOrder(m_peakwidthSpin, m_numpointsSpin);
  setTabOrder(m_numpointsSpin, m_max2thetaSpin);
  setTabOrder(m_max2thetaSpin, m_resetParametersButton);
  setTabOrder(m_resetParametersButton, m_resetViewButton);
  setTabOrder(m_resetViewButton, m_saveDataButton);
  setTabOrder(m_saveDataButton, m_saveImageButton);
  setTabOrder(m_saveImageButton, m_closeButton);
  connect(m_wavelengthSpin, SIGNAL(valueChanged(double)), this, SLOT(updatePlot()));
  connect(m_peakwidthSpin, SIGNAL(valueChanged(double)), this, SLOT(updatePlot()));
  connect(m_numpointsSpin, SIGNAL(valueChanged(int)), this, SLOT(updatePlot()));
  connect(m_max2thetaSpin, SIGNAL(valueChanged(double)), this, SLOT(updatePlot()));
  connect(m_resetParametersButton, &QPushButton::clicked, this, &XrdViewDialog::resetParameters);
  connect(m_resetViewButton, &QPushButton::clicked, this, &XrdViewDialog::resetView);
  connect(m_saveDataButton, &QPushButton::clicked, this, &XrdViewDialog::saveData);
  connect(m_saveImageButton, &QPushButton::clicked, this, &XrdViewDialog::saveImage);
  connect(m_closeButton, &QPushButton::clicked, this, &XrdViewDialog::close);
}

XrdViewDialog::~XrdViewDialog() = default;

void XrdViewDialog::displayStructure(const Atoms::Geometry& structure, const QString& label)
{
  m_geometry = structure;
  m_structureLabel = label;
  m_hasStructure = true;
  updatePlot();
  show();
  raise();
  activateWindow();
}

void XrdViewDialog::resetParameters()
{
  m_wavelengthSpin->setValue(DEFAULT_WAVELENGTH);
  m_peakwidthSpin->setValue(DEFAULT_PEAKWIDTH);
  m_numpointsSpin->setValue(DEFAULT_NUMPOINTS);
  m_max2thetaSpin->setValue(DEFAULT_MAX_2THETA);
}

void XrdViewDialog::updatePlot()
{
  if (!m_hasStructure)
    return;

  m_lastWavelength = m_wavelengthSpin->value();
  m_lastPeakwidth = m_peakwidthSpin->value();
  m_lastNumpoints = static_cast<size_t>(m_numpointsSpin->value());
  m_lastMax2theta = m_max2thetaSpin->value();

  XrdData results;
  const QString tag = m_structureLabel;
  const QString formula = m_geometry.getChemicalFormula();
  if (!generatePattern(m_geometry, results, m_lastWavelength, m_lastPeakwidth,
                       m_lastNumpoints, m_lastMax2theta)) {
    Common::debug(QString("%1: XRD generation failed for structure %2")
                 .arg(__func__).arg(tag));
    return;
  }

  m_lastData = results;
  m_lastTag = tag;
  m_infoLabel->setText(
    tr("%1%2%3")
      .arg(tag.isEmpty() ? QString() : tag)
      .arg(tag.isEmpty() || formula.isEmpty() ? QString() : "  -  ")
      .arg(formula));
  setWindowTitle(tag.isEmpty() ? tr("Simulated XRD Pattern")
                   : tr("Simulated XRD Pattern - %1").arg(tag));
  m_plot->clearPlotCurves();
  m_plot->addXrdData(results);
  m_saveDataButton->setEnabled(!m_lastData.empty());
  m_saveImageButton->setEnabled(true);
  resetView();
}

void XrdViewDialog::resetView()
{
  m_plot->setAxisAutoScale(QwtPlot::yLeft);
  m_plot->setAxisAutoScale(QwtPlot::xBottom);
  m_plot->replot();
}

void XrdViewDialog::saveImage() const
{
  const QString defaultName = m_lastTag.isEmpty()
                                ? QStringLiteral("xrd-pattern.png")
                                : m_lastTag + QStringLiteral("-xrd.png");
  QString filename = QFileDialog::getSaveFileName(
    const_cast<XrdViewDialog*>(this), tr("Save XRD Image"),
    QDir(defaultSaveDir()).filePath(defaultName),
    tr("PNG Image (*.png);;JPEG Image (*.jpg);;BMP Image (*.bmp)"),
    nullptr, QFileDialog::DontUseNativeDialog);
  if (filename.isEmpty())
    return;

  const int exportScale = 2;
  QImage image(m_plot->size() * exportScale, QImage::Format_ARGB32_Premultiplied);
  image.fill(Qt::white);

  QPainter painter(&image);
  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.setRenderHint(QPainter::TextAntialiasing, true);
  painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
  painter.scale(exportScale, exportScale);
  m_plot->render(&painter, QPoint(), QRegion(), QWidget::DrawChildren);
  painter.end();

  if (!image.save(filename, nullptr, 95)) {
    QMessageBox::warning(const_cast<XrdViewDialog*>(this), tr("Save XRD Image"),
                         tr("Could not write image file:\n%1").arg(filename));
  }
}

void XrdViewDialog::saveData() const
{
  if (m_lastData.empty())
    return;

  const QString defaultName = m_lastTag.isEmpty()
                                ? QStringLiteral("xrd-data.txt")
                                : m_lastTag + QStringLiteral("-xrd.txt");
  QString filename = QFileDialog::getSaveFileName(
    const_cast<XrdViewDialog*>(this), tr("Save XRD Data"),
    QDir(defaultSaveDir()).filePath(defaultName),
    tr("Text files (*.txt);;Data files (*.dat);;All files (*.*)"),
    nullptr, QFileDialog::DontUseNativeDialog);
  if (filename.isEmpty())
    return;

  QFile file(filename);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QMessageBox::warning(const_cast<XrdViewDialog*>(this), tr("Save XRD Data"),
                         tr("Could not open '%1' for writing.").arg(filename));
    return;
  }

  QTextStream stream(&file);
  if (!m_lastTag.isEmpty())
    stream << "# structure=\"" << m_lastTag << "\"\n";
  stream << "# wavelength=" << m_lastWavelength
         << " peakwidth=" << m_lastPeakwidth
         << " numpoints=" << m_lastNumpoints
         << " max2theta=" << m_lastMax2theta << "\n";
  stream << "# 2theta intensity\n";
  for (const auto& point : m_lastData)
    stream << QString("%1 %2\n")
                .arg(point.first, 20, 'f', 6)
                .arg(point.second, 20, 'f', 6);
}

} // namespace GenerateXrd
