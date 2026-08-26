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

#include "ui_xrdviewdialog.h"

#include <common/compatibility/platform_defs.h>
#include <generatexrd/gui/xrd_plot.h>
#include <common/output.h>

#include <QCloseEvent>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QImage>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QTextStream>

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
    ui(new Ui::XrdViewDialog),
    m_hasStructure(false),
    m_lastWavelength(DEFAULT_WAVELENGTH),
    m_lastPeakwidth(DEFAULT_PEAKWIDTH),
    m_lastNumpoints(DEFAULT_NUMPOINTS),
    m_lastMax2theta(DEFAULT_MAX_2THETA)
{
  ui->setupUi(this);

  // The spin box limits come from the design; their starting values are the
  //   pattern defaults, which resetParameters() also uses.
  ui->wavelengthSpin->setValue(m_lastWavelength);
  ui->peakwidthSpin->setValue(m_lastPeakwidth);
  ui->numpointsSpin->setValue(static_cast<int>(m_lastNumpoints));
  ui->max2thetaSpin->setValue(m_lastMax2theta);

  connect(ui->wavelengthSpin, SIGNAL(valueChanged(double)), this, SLOT(updatePlot()));
  connect(ui->peakwidthSpin, SIGNAL(valueChanged(double)), this, SLOT(updatePlot()));
  connect(ui->numpointsSpin, SIGNAL(valueChanged(int)), this, SLOT(updatePlot()));
  connect(ui->max2thetaSpin, SIGNAL(valueChanged(double)), this, SLOT(updatePlot()));
  connect(ui->resetParametersButton, &QPushButton::clicked, this, &XrdViewDialog::resetParameters);
  connect(ui->resetViewButton, &QPushButton::clicked, this, &XrdViewDialog::resetView);
  connect(ui->saveDataButton, &QPushButton::clicked, this, &XrdViewDialog::saveData);
  connect(ui->saveImageButton, &QPushButton::clicked, this, &XrdViewDialog::saveImage);
  connect(ui->closeButton, &QPushButton::clicked, this, &XrdViewDialog::close);
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
  ui->wavelengthSpin->setValue(DEFAULT_WAVELENGTH);
  ui->peakwidthSpin->setValue(DEFAULT_PEAKWIDTH);
  ui->numpointsSpin->setValue(DEFAULT_NUMPOINTS);
  ui->max2thetaSpin->setValue(DEFAULT_MAX_2THETA);
}

void XrdViewDialog::updatePlot()
{
  if (!m_hasStructure)
    return;

  // Save the shown parameters, so an exported pattern has the same values.
  m_lastWavelength = ui->wavelengthSpin->value();
  m_lastPeakwidth = ui->peakwidthSpin->value();
  m_lastNumpoints = static_cast<size_t>(ui->numpointsSpin->value());
  m_lastMax2theta = ui->max2thetaSpin->value();

  XrdData results;
  const QString tag = m_structureLabel;
  const QString formula = m_geometry.getChemicalFormula();
  if (!generatePattern(m_geometry, results, m_lastWavelength, m_lastPeakwidth,
                       m_lastNumpoints, m_lastMax2theta)) {
    Common::debug(QString("%1: XRD generation failed for structure %2")
                 .arg(__func__).arg(tag));
    return;
  }

  // Keep this result for both the plot and later data export.
  m_lastData = results;
  m_lastTag = tag;

  // Update the complete visible result after a successful calculation.
  ui->infoLabel->setText(
    tr("%1%2%3")
      .arg(tag.isEmpty() ? QString() : tag)
      .arg(tag.isEmpty() || formula.isEmpty() ? QString() : "  -  ")
      .arg(formula));
  setWindowTitle(tag.isEmpty() ? tr("Simulated XRD Pattern")
                   : tr("Simulated XRD Pattern - %1").arg(tag));
  ui->plot->clearPlotCurves();
  ui->plot->addXrdData(results);
  ui->saveDataButton->setEnabled(!m_lastData.empty());
  ui->saveImageButton->setEnabled(true);
  resetView();
}

void XrdViewDialog::resetView()
{
  ui->plot->setAxisAutoScale(QwtPlot::yLeft);
  ui->plot->setAxisAutoScale(QwtPlot::xBottom);
  ui->plot->replot();
}

void XrdViewDialog::saveImage() const
{
  const QString defaultName = m_lastTag.isEmpty()
                                ? QStringLiteral("xrd-pattern.png")
                                : m_lastTag + QStringLiteral("-xrd.png");
  QString filename = QFileDialog::getSaveFileName(
    const_cast<XrdViewDialog*>(this), tr("Save XRD View"),
    QDir(defaultSaveDir()).filePath(defaultName),
    tr("PNG Image (*.png);;JPEG Image (*.jpg);;BMP Image (*.bmp)"),
    nullptr, QFileDialog::DontUseNativeDialog);
  if (filename.isEmpty())
    return;

  const int exportScale = 2;
  QImage image(ui->plot->size() * exportScale, QImage::Format_ARGB32_Premultiplied);
  image.fill(Qt::white);

  QPainter painter(&image);
  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.setRenderHint(QPainter::TextAntialiasing, true);
  painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
  painter.scale(exportScale, exportScale);
  ui->plot->render(&painter, QPoint(), QRegion(), QWidget::DrawChildren);
  painter.end();

  if (!image.save(filename, nullptr, 95)) {
    QMessageBox::warning(const_cast<XrdViewDialog*>(this), tr("Save XRD View"),
                         tr("Could not write image file:\n%1").arg(filename));
  }
}

void XrdViewDialog::saveData() const
{
  if (m_lastData.empty())
    return;

  const QString defaultName = m_lastTag.isEmpty()
                                ? QStringLiteral("xrd-data.dat")
                                : m_lastTag + QStringLiteral("-xrd.dat");
  QString filename = QFileDialog::getSaveFileName(
    const_cast<XrdViewDialog*>(this), tr("Save XRD Data"),
    QDir(defaultSaveDir()).filePath(defaultName),
    tr("Data files (*.dat);;Text files (*.txt)"),
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
