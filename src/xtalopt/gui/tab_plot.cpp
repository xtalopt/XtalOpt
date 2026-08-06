/**********************************************************************
  TabPlot - The plot tab: charts of the search results.

  Copyright (C) 2009-2011 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <xtalopt/gui/tab_plot.h>
#include <xtalopt/gui/xtalopt_plot.h>

#include <xtalopt/structures/xtal.h>
#include <xtalopt/gui/dialog.h>
#include <xtalopt/xtalopt.h>

#include <common/compatibility/platform_compat.h>
#include <common/constants.h>
#include <search/queuemanager.h>
#include <search/tracker.h>
#include <common/fileutils.h>
#include <atoms/formats/poscarformat.h>

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QCursor>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QImage>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QRectF>
#include <QReadWriteLock>
#include <QSettings>
#include <QSignalBlocker>

#include <qwt_plot_canvas.h>
#include <qwt_plot_renderer.h>
#include <qwt_plot_textlabel.h>
#include <qwt_scale_map.h>
#include <qwt_scale_engine.h>
#include <qwt_text.h>

#include <float.h>
#include <sstream>

using namespace Search;

namespace XtalOpt {

namespace {

QString defaultSaveDir()
{
#if GS_WINDOWS
  return QStringLiteral("C:/");
#else
  return QDir::homePath();
#endif
}

enum PlotStatusCategory
{
  Psc_Complete,
  Psc_Dismissed,
  Psc_Similar,
  Psc_Failed,
  Psc_Incomplete
};

PlotStatusCategory plotStatusCategory(const Xtal* xtal)
{
  const Structure::State state = xtal->getStatus();
  if (xtal->isSimilar()) // This is not a "status"; so comes first!
    return Psc_Similar;
  if (state == Structure::Optimized)
    return Psc_Complete;
  if (xtal->isDismissedFinalState())
    return Psc_Dismissed;
  if (xtal->isStoppedFinalState())
    return Psc_Failed;

  return Psc_Incomplete;
}

QRectF plotTextLabelRect(XtalOptPlot* plot, const QwtPlotTextLabel* label)
{
  if (!label->isVisible())
    return QRectF();

  const QwtText text = label->text();
  const QSizeF size = text.textSize(plot->canvas()->font());
  const int margin = label->margin();
  const QRectF area = plot->canvas()->contentsRect().adjusted(margin, margin,
                                                              -margin, -margin);
  qreal left = area.center().x() - size.width() / 2.0;
  qreal top = area.center().y() - size.height() / 2.0;
  if (text.renderFlags() & Qt::AlignLeft)
    left = area.left();
  else if (text.renderFlags() & Qt::AlignRight)
    left = area.right() - size.width();
  if (text.renderFlags() & Qt::AlignTop)
    top = area.top();
  else if (text.renderFlags() & Qt::AlignBottom)
    top = area.bottom() - size.height();
  return QRectF(left, top, size.width(), size.height());
}

void placePlotLabels(XtalOptPlot* plot, const QRectF& textLabelRect)
{
  // This functions tries to place labels in a more smart way to avoid overlaps!
  // It:
  //   Tries above, below, right, left, then four corner positions,
  //   Avoids existing plot symbols and previously placed labels,
  //   Keeps labels inside the plot canvas,
  //   Retains Qwt's default placement if no collision-free position exists.

  const Qt::Alignment positions[] = {
    Qt::AlignBottom, Qt::AlignTop, Qt::AlignRight, Qt::AlignLeft,
    Qt::AlignTop | Qt::AlignRight, Qt::AlignTop | Qt::AlignLeft,
    Qt::AlignBottom | Qt::AlignRight, Qt::AlignBottom | Qt::AlignLeft
  };
  const QRectF canvasRect = plot->canvas()->contentsRect();
  const QwtScaleMap xMap = plot->canvasMap(QwtPlot::xBottom);
  const QwtScaleMap yMap = plot->canvasMap(QwtPlot::yLeft);
  QList<QRectF> usedRects;
  if (!textLabelRect.isEmpty())
    usedRects.append(textLabelRect);

  const std::vector<std::unique_ptr<QwtPlotMarker>>& markers = plot->plotMarkers();
  for (size_t i = 0; i < markers.size(); ++i) {
    const QwtPlotMarker* marker = markers[i].get();
    if (marker->symbol()) {
      const QSize symbolSize = marker->symbol()->size();
      const QPointF point(xMap.transform(marker->xValue()),
                          yMap.transform(marker->yValue()));
      usedRects.append(QRectF(point.x() - symbolSize.width() / 2.0,
                              point.y() - symbolSize.height() / 2.0,
                              symbolSize.width(), symbolSize.height()));
    }
  }

  for (size_t i = 0; i < markers.size(); ++i) {
    QwtPlotMarker* marker = markers[i].get();
    if (marker->label().text().isEmpty())
      continue;

    const QPointF point(xMap.transform(marker->xValue()),
                        yMap.transform(marker->yValue()));
    const QSizeF textSize = marker->label().textSize(plot->canvas()->font());
    const QSize symbolSize = marker->symbol() ? marker->symbol()->size() : QSize();
    const qreal xGap = symbolSize.width() / 2.0 + marker->spacing();
    const qreal yGap = symbolSize.height() / 2.0 + marker->spacing();
    marker->setLabelAlignment(Qt::AlignBottom);

    for (size_t j = 0; j < sizeof(positions) / sizeof(positions[0]); ++j) {
      qreal left = point.x() - textSize.width() / 2.0;
      qreal top = point.y() - textSize.height() / 2.0;

      if (positions[j] & Qt::AlignLeft)
        left = point.x() - xGap - textSize.width();
      else if (positions[j] & Qt::AlignRight)
        left = point.x() + xGap;
      if (positions[j] & Qt::AlignTop)
        top = point.y() - yGap - textSize.height();
      else if (positions[j] & Qt::AlignBottom)
        top = point.y() + yGap;

      const QRectF labelRect(left, top, textSize.width(), textSize.height());
      bool overlaps = !canvasRect.contains(labelRect);
      for (int k = 0; !overlaps && k < usedRects.size(); ++k)
        overlaps = labelRect.intersects(usedRects[k]);
      if (!overlaps) {
        marker->setLabelAlignment(positions[j]);
        usedRects.append(labelRect);
        break;
      }
    }
  }
}

} // namespace

TabPlot::TabPlot(Search::AbstractDialog* parent, XtalOpt* p)
  : AbstractTab(parent, p), m_enablePlotUpdate(true),
    m_plot_mutex(new QReadWriteLock(QReadWriteLock::Recursive)),
    m_guideLabel(nullptr), m_context_xtal(nullptr)
{
  ui.setupUi(m_tab_widget);
  ui.combo_xAxis->setCurrentIndex(StructureINDX_T);
  ui.combo_yAxis->setCurrentIndex(AboveHull_per_Atm_T);
  ui.combo_labelType->setCurrentIndex(StructureTAG_L);
  ui.cb_labelPoints->setChecked(true);

  // Change the margins on the plot for autoscaling
  ui.plot->axisScaleEngine(QwtPlot::yLeft)->setMargins(0.0, 0.0);
  ui.plot->axisScaleEngine(QwtPlot::yLeft)
    ->setAttribute(QwtScaleEngine::Floating);
  ui.plot->axisScaleEngine(QwtPlot::xBottom)->setMargins(0.0, 0.0);
  ui.plot->axisScaleEngine(QwtPlot::xBottom)
    ->setAttribute(QwtScaleEngine::Floating);

  QString guideText = QStringLiteral(
    "<span style='color:#0000ff'>&#9679;</span> %1&nbsp;&nbsp;&nbsp;"
    "<span style='color:#008000'>&#9632;</span> %2&nbsp;&nbsp;&nbsp;"
    "<span style='color:#808080'>&#9670;</span> %3&nbsp;&nbsp;&nbsp;"
    "<span style='color:#ff8c00'>&#9650;</span> %4&nbsp;&nbsp;&nbsp;"
    "<span style='color:#ff0000'>&#9660;</span> %5");
  guideText = guideText.arg(tr("Complete"))
                       .arg(tr("Similar"))
                       .arg(tr("Dismissed"))
                       .arg(tr("Incomplete"))
                       .arg(tr("Failed"));
  QwtText guide(guideText, QwtText::RichText);
  guide.setRenderFlags(Qt::AlignHCenter | Qt::AlignTop);
  guide.setBackgroundBrush(QColor(255, 255, 255, 220));
  guide.setBorderPen(QPen(Qt::lightGray));

  m_guideLabel = new QwtPlotTextLabel;
  m_guideLabel->setText(guide);
  m_guideLabel->setMargin(5);
  m_guideLabel->setZ(100.0);
  m_guideLabel->attach(ui.plot);

  // dialog connections
  connect(m_dialog, &Search::AbstractDialog::selectedGeometryChanged,
          this, &TabPlot::highlightXtal);

  // Plot connections
  connect(ui.push_refresh, &QPushButton::clicked, this, &TabPlot::refreshPlot);
  connect(ui.push_savePlot, &QPushButton::clicked, this, &TabPlot::savePlotImage);
  connect(ui.combo_xAxis, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          this, &TabPlot::refreshPlot);
  connect(ui.combo_yAxis, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          this, &TabPlot::refreshPlot);
  connect(ui.cb_labelPoints,    &QCheckBox::toggled, this, &TabPlot::updatePlot);
  connect(ui.cb_showLegend, &QCheckBox::toggled, this, &TabPlot::updatePlotLayout);
  connect(ui.cb_smartLabelPlacement, &QCheckBox::toggled, this, &TabPlot::updatePlot);
  connect(ui.combo_labelType,
          static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          this, &TabPlot::updatePlot);
  connect(ui.cb_showComplete,     &QCheckBox::toggled, this, &TabPlot::updatePlot);
  connect(ui.cb_showSimilarities, &QCheckBox::toggled, this, &TabPlot::updatePlot);
  connect(ui.cb_showDismissed,    &QCheckBox::toggled, this, &TabPlot::updatePlot);
  connect(ui.cb_showIncompletes,  &QCheckBox::toggled, this, &TabPlot::updatePlot);
  connect(ui.cb_showFailures,     &QCheckBox::toggled, this, &TabPlot::updatePlot);
  connect(ui.plot, &XtalOptPlot::selectedMarkerChanged, this, &TabPlot::selectXtal);
  connect(ui.plot, &XtalOptPlot::plotResized, this, &TabPlot::updatePlotLayout);
  ui.plot->canvas()->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(ui.plot->canvas(), &QWidget::customContextMenuRequested, this, &TabPlot::plotContextMenu);
  connect(m_search, &Search::SearchBase::refreshAllStructureInfo, this, &TabPlot::updatePlot);
  connect(m_search, &Search::SearchBase::structureViewDataChanged, this, &TabPlot::updatePlot);
  connect(m_search->queue(), &Search::QueueManager::structureUpdated,
          this, [this](Search::Structure*) { updatePlot(); });
  connect(m_search->tracker(), &Search::Tracker::newStructureAdded,
          this, [this](Search::Structure*) { updatePlot(); });

  connect(m_search, &Search::SearchBase::structureViewUpdateBlocked,
          this, [this](bool blocked) {
            if (blocked)
              disablePlotUpdate();
            else
              enablePlotUpdate();
          });
  connect(m_search, &Search::SearchBase::structuresAboutToBeDeleted,
          this, &TabPlot::releaseStructureReferences);

  initialize();
}

TabPlot::~TabPlot()
{
  releaseStructureReferences();
  delete m_plot_mutex;
}

void TabPlot::disconnectGUI()
{
  ui.push_refresh->disconnect();
  ui.combo_xAxis->disconnect();
  ui.combo_yAxis->disconnect();
  ui.cb_labelPoints->disconnect();
  ui.cb_showLegend->disconnect();
  ui.cb_smartLabelPlacement->disconnect();
  ui.combo_labelType->disconnect();
  ui.cb_showComplete->disconnect();
  ui.cb_showSimilarities->disconnect();
  ui.cb_showDismissed->disconnect();
  ui.cb_showIncompletes->disconnect();
  ui.cb_showFailures->disconnect();
  this->disconnect();
  disconnect(m_dialog, 0, this, 0);
  disconnect(m_search, 0, this, 0);
}

void TabPlot::refreshPlot()
{
  // Reset axis scales and then update
  ui.plot->setAxisAutoScale(QwtPlot::yLeft);
  ui.plot->setAxisAutoScale(QwtPlot::xBottom);
  updatePlot();
}

void TabPlot::updatePlotLayout()
{
  const QSignalBlocker plotBlocker(ui.plot);
  m_guideLabel->setVisible(ui.cb_showLegend->isChecked());
  const QRectF guideRect = plotTextLabelRect(ui.plot, m_guideLabel);
  if (ui.plot->axisAutoScale(QwtPlot::yLeft)) {
    QwtScaleEngine* yScaleEngine = ui.plot->axisScaleEngine(QwtPlot::yLeft);
    yScaleEngine->setMargins(0.0, 0.0);
    ui.plot->replot();

    const QwtScaleDiv yScale = ui.plot->axisScaleDiv(QwtPlot::yLeft);
    const double range = qAbs(yScale.upperBound() - yScale.lowerBound());
    const QRectF canvasRect = ui.plot->canvas()->contentsRect();
    const double topPart = canvasRect.height() > 0.0
      ? (guideRect.bottom() - canvasRect.top()) / canvasRect.height() : 0.0;
    double bottomPart = 0.0;
    if (ui.cb_labelPoints->isChecked() && canvasRect.height() > 0.0) {
      const std::vector<std::unique_ptr<QwtPlotMarker>>& markers = ui.plot->plotMarkers();
      for (size_t i = 0; i < markers.size(); ++i) {
        const QwtPlotMarker* marker = markers[i].get();
        if (marker->label().text().isEmpty())
          continue;
        const QSize symbolSize = marker->symbol() ? marker->symbol()->size() : QSize();
        const double labelHeight = marker->label().textSize(ui.plot->canvas()->font()).height();
        bottomPart = (labelHeight + symbolSize.height() / 2.0 + marker->spacing()) /
                     canvasRect.height();
        break;
      }
    }
    yScaleEngine->setMargins(range * (0.03 + bottomPart),
                             range * (0.03 + qMax(0.0, topPart)));
    ui.plot->replot();
  }

  if (ui.cb_labelPoints->isChecked() &&
      ui.cb_smartLabelPlacement->isChecked()) {
    placePlotLabels(ui.plot, guideRect);
    ui.plot->replot();
  }
}

void TabPlot::releaseStructureReferences()
{
  QWriteLocker locker(m_plot_mutex);
  m_context_xtal = nullptr;
  m_marker_xtal_map.clear();
  ui.plot->clearAll();
}

void TabPlot::savePlotImage()
{
  QString selectedFilter;
  QString filename = QFileDialog::getSaveFileName(m_tab_widget, tr("Save Plot Image"),
     QDir(defaultSaveDir()).filePath("xtalopt-plot.png"),
    tr("PNG Image (*.png);;JPEG Image (*.jpg *.jpeg);;BMP Image (*.bmp)"), &selectedFilter,
    QFileDialog::DontUseNativeDialog);
  if (filename.isEmpty())
    return;

  if (QFileInfo(filename).suffix().isEmpty()) {
    if (selectedFilter.contains("*.jpg"))
      filename += ".jpg";
    else if (selectedFilter.contains("*.bmp"))
      filename += ".bmp";
    else
      filename += ".png";
  }

  QByteArray format = QFileInfo(filename).suffix().toLower().toLatin1();
  if (format == "jpg" || format == "jpeg")
    format = "JPG";
  else if (format == "bmp")
    format = "BMP";
  else
    format = "PNG";

  ui.plot->replot();

  const int exportScale = 4;
  QImage image(ui.plot->size() * exportScale, QImage::Format_ARGB32_Premultiplied);
  image.fill(Qt::white);

  QPainter painter(&image);
  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.setRenderHint(QPainter::TextAntialiasing, true);
  painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
  painter.scale(exportScale, exportScale);

  QwtPlotRenderer renderer;
  renderer.render(ui.plot, &painter, QRectF(QPointF(0.0, 0.0), QSizeF(ui.plot->size())));
  painter.end();

  if (!image.save(filename, format.constData(), 95)) {
    QMessageBox::warning(m_tab_widget,
                         tr("Save Plot Image"),
                         tr("Could not write image file:\n%1")
                           .arg(filename));
  }
}

void TabPlot::updatePlotFonts()
{
  const QFont baseFont = ui.plot->font();
  QFont titleFont = baseFont;
  titleFont.setBold(true);

  ui.plot->setAxisFont(QwtPlot::xBottom, baseFont);
  ui.plot->setAxisFont(QwtPlot::yLeft, baseFont);

  const int axes[] = { QwtPlot::xBottom, QwtPlot::yLeft };
  for (int i = 0; i < 2; ++i) {
    const int axis = axes[i];
    QwtText title = ui.plot->axisTitle(axis);
    title.setFont(titleFont);
    ui.plot->setAxisTitle(axis, title);
  }

  QwtText guide = m_guideLabel->text();
  guide.setFont(baseFont);
  m_guideLabel->setText(guide);

  const std::vector<std::unique_ptr<QwtPlotMarker>>& markers = ui.plot->plotMarkers();
  for (size_t i = 0; i < markers.size(); ++i) {
    QwtText label = markers[i]->label();
    label.setFont(baseFont);
    markers[i]->setLabel(label);
  }
}

void TabPlot::updatePlot()
{
  // If we have disabled plot updating, just return.
  if (!m_enablePlotUpdate)
    return;

  updateGUI();
  if (!m_search)
    return;
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);
  if (!xtalopt)
    return;

  // Make sure we have structures!
  if (m_search->tracker()->size() == 0)
    return;

  // Lock plot mutex
  QWriteLocker plotLocker(m_plot_mutex);

  // Here, we want to make sure that objectives are shown only if they are present
  // To show the proper number of them, we set the starting point to Objectivei_*
  const int userObjectivesNum = xtalopt->getUserObjectivesNum();
  const int constraintsNum = xtalopt->getConstraintsNum();
  const int dynamicValuesNum = userObjectivesNum + constraintsNum;
  int numaxisitems = dynamicValuesNum + Objectivei_T;
  int numsymbitems = dynamicValuesNum + Objectivei_L;
  const int xSelection = ui.combo_xAxis->currentIndex();
  const int ySelection = ui.combo_yAxis->currentIndex();
  const int labelSelection = ui.combo_labelType->currentIndex();
  const QSignalBlocker xBlocker(ui.combo_xAxis);
  const QSignalBlocker yBlocker(ui.combo_yAxis);
  const QSignalBlocker labelBlocker(ui.combo_labelType);
  while (ui.combo_xAxis->count() > Objectivei_T)
    ui.combo_xAxis->removeItem(Objectivei_T);
  while (ui.combo_yAxis->count() > Objectivei_T)
    ui.combo_yAxis->removeItem(Objectivei_T);
  while (ui.combo_labelType->count() > Objectivei_L)
    ui.combo_labelType->removeItem(Objectivei_L);
  ui.combo_xAxis->setMaxCount(numaxisitems);
  ui.combo_yAxis->setMaxCount(numaxisitems);
  ui.combo_labelType->setMaxCount(numsymbitems);
  for (int i = 0; i < userObjectivesNum; i++) {
      ui.combo_xAxis->addItem(tr("Objective%1").arg(i+1));
      ui.combo_yAxis->addItem(tr("Objective%1").arg(i+1));
      ui.combo_labelType->addItem(tr("Objective%1").arg(i+1));
  }
  for (int i = 0; i < constraintsNum; i++) {
      ui.combo_xAxis->addItem(tr("Constraint%1").arg(i+1));
      ui.combo_yAxis->addItem(tr("Constraint%1").arg(i+1));
      ui.combo_labelType->addItem(tr("Constraint%1").arg(i+1));
  }
  ui.combo_xAxis->setCurrentIndex(qMin(xSelection, ui.combo_xAxis->count() - 1));
  ui.combo_yAxis->setCurrentIndex(qMin(ySelection, ui.combo_yAxis->count() - 1));
  ui.combo_labelType->setCurrentIndex(qMin(labelSelection, ui.combo_labelType->count() - 1));

  plotTrends();

}

void TabPlot::plotTrends()
{
  // Don't replot while we are updating
  ui.plot->setAutoReplot(false);

  // Store the selected xtal
  Xtal* selectedXtal = m_marker_xtal_map[ui.plot->selectedMarker()];

  // There may be a faster way to do this that doesn't include deleting
  // and making the whole plot over again, but for now, this works. We
  // just delete the whole plot and make it over again...
  m_marker_xtal_map.clear();
  ui.plot->clearAll();

  double x, y;
  int ind;
  Xtal* xtal = nullptr;
  double minE = DBL_MAX;
  int lastTraceStructure = -1;
  bool performTrace = false;
  // Load config settings:
  bool labelPoints = ui.cb_labelPoints->isChecked();
  bool showComplete = ui.cb_showComplete->isChecked();
  bool showSimilarities = ui.cb_showSimilarities->isChecked();
  bool showDismissed = ui.cb_showDismissed->isChecked();
  bool showIncompletes = ui.cb_showIncompletes->isChecked();
  bool showFailures = ui.cb_showFailures->isChecked();
  LabelTypes labelType = LabelTypes(ui.combo_labelType->currentIndex());
  PlotAxes xAxis = PlotAxes(ui.combo_xAxis->currentIndex());
  PlotAxes yAxis = PlotAxes(ui.combo_yAxis->currentIndex());
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);
  if (!xtalopt)
    return;
  const int userObjectivesNum = xtalopt->getUserObjectivesNum();

  if (xAxis == StructureINDX_T && (yAxis == Energy_T || yAxis == Enthalpy_T ||
                               yAxis == Enthalpy_per_Atm_T || yAxis == AboveHull_per_Atm_T)) {
    performTrace = true;
  }

  QList<Structure*> structures;
  {
    QReadLocker trackerLocker(m_search->tracker()->rwLock());
    structures.reserve(m_search->tracker()->list()->size());
    for (auto* structure : *m_search->tracker()->list())
      structures.append(structure);
  }
  for (int i = 0; i < structures.size(); i++) {
    x = y = 0;
    xtal = qobject_cast<Xtal*>(structures[i]);
    if (!xtal)
      continue;
    QReadLocker xtalLocker(&xtal->lock());
    const PlotStatusCategory statusCategory = plotStatusCategory(xtal);

    if (statusCategory == Psc_Complete && !showComplete) {
      continue;
    }

    if (statusCategory == Psc_Similar && !showSimilarities) {
      continue;
    }

    if (statusCategory == Psc_Dismissed && !showDismissed) {
      continue;
    }

    if (statusCategory == Psc_Failed && !showFailures) {
      continue;
    }

    if (statusCategory == Psc_Incomplete && !showIncompletes) {
      continue;
    }

    int xtalNumAtoms = xtal->numAtoms();
    bool usePoint = true;
    // Get X/Y data
    for (int j = 0; j < 2; j++) { // 0 = x, 1 = y
      switch (j) {
        case 0:
          ind = xAxis;
          break;
        default:
          ind = yAxis;
          break;
      }

      switch (ind) {
        case StructureINDX_T:
          switch (j) {
            case 0:
              x = xtal->getIndex();
              break;
            default:
              y = xtal->getIndex();
              break;
          }
          break;
        case Generation_T:
          switch (j) {
            case 0:
              x = xtal->getGeneration();
              break;
            default:
              y = xtal->getGeneration();
              break;
          }
          break;
        case Enthalpy_T:
          // Skip xtals that don't have enthalpy/energy set
          if (xtal->getEnergy() == 0.0 && !xtal->hasEnthalpy()) {
            usePoint = false;
            continue;
          }
          switch (j) {
            case 0:
              x = xtal->getEnthalpy();
              break;
            default:
              y = xtal->getEnthalpy();
              break;
          }
          break;
        case ParetoFront_T:
          // Skip xtals that don't have pareto front
          if (xtal->getParetoFront() < 0) {
            usePoint = false;
            continue;
          }
          switch (j) {
            case 0:
              x = xtal->getParetoFront();
              break;
            default:
              y = xtal->getParetoFront();
              break;
          }
          break;
        case AtomCount_T:
          // Skip xtals that don't have pareto front
          if (xtal->numAtoms() <= 0) {
            usePoint = false;
            continue;
          }
          switch (j) {
            case 0:
              x = xtal->numAtoms();
              break;
            default:
              y = xtal->numAtoms();
              break;
          }
          break;
        case AboveHull_per_Atm_T:
          // Skip xtals that don't have above hull
          if (GS_ISNAN(xtal->getDistAboveHull())) {
            usePoint = false;
            continue;
          }
          switch (j) {
            case 0:
              x = xtal->getDistAboveHull();
              break;
            default:
              y = xtal->getDistAboveHull();
              break;
          }
          break;
        case Enthalpy_per_Atm_T:
          // Skip xtals that don't have enthalpy/energy set
          if (xtal->getEnergy() == 0.0 && !xtal->hasEnthalpy()) {
            usePoint = false;
            continue;
          }
          switch (j) {
            case 0:
              x = xtal->getEnthalpyPerAtom();
              break;
            default:
              y = xtal->getEnthalpyPerAtom();
              break;
          }
          break;
        case Energy_T:
          // Skip xtals that don't have energy set
          if (xtal->getEnergy() == 0.0) {
            usePoint = false;
            continue;
          }
          switch (j) {
            case 0:
              x = xtal->getEnergy();
              break;
            default:
              y = xtal->getEnergy();
              break;
          }
          break;
        case PV_T:
          // Skip xtals that don't have enthalpy/energy set
          if (xtal->getEnergy() == 0.0 && !xtal->hasEnthalpy()) {
            usePoint = false;
            continue;
          }
          switch (j) {
            case 0:
              x = xtal->getPV();
              break;
            default:
              y = xtal->getPV();
              break;
          }
          break;
        case A_per_Atm_T:
          switch (j) {
            case 0:
              x = xtal->getA() / xtalNumAtoms;
              break;
            default:
              y = xtal->getA() / xtalNumAtoms;
              break;
          }
          break;
        case B_per_Atm_T:
          switch (j) {
            case 0:
              x = xtal->getB() / xtalNumAtoms;
              break;
            default:
              y = xtal->getB() / xtalNumAtoms;
              break;
          }
          break;
        case C_per_Atm_T:
          switch (j) {
            case 0:
              x = xtal->getC() / xtalNumAtoms;
              break;
            default:
              y = xtal->getC() / xtalNumAtoms;
              break;
          }
          break;
        case Alpha_T:
          switch (j) {
            case 0:
              x = xtal->getAlpha();
              break;
            default:
              y = xtal->getAlpha();
              break;
          }
          break;
        case Beta_T:
          switch (j) {
            case 0:
              x = xtal->getBeta();
              break;
            default:
              y = xtal->getBeta();
              break;
          }
          break;
        case Gamma_A:
          switch (j) {
            case 0:
              x = xtal->getGamma();
              break;
            default:
              y = xtal->getGamma();
              break;
          }
          break;
        case Volume_T:
          switch (j) {
            case 0:
              x = xtal->getVolume();
              break;
            default:
              y = xtal->getVolume();
              break;
          }
          break;
        case Volume_per_Atm_T:
          switch (j) {
            case 0:
              x = xtal->getVolumePerAtom();
              break;
            default:
              y = xtal->getVolumePerAtom();
              break;
          }
          break;
        default:
          // Objectives in multi-objective run. Since there is no fixed number of
          //   objectives; and MSVC does not support "case range", we put them
          //   under "default".
          // Their index in the list of options starts from Objectivei_T;
          //   getUserObjectiveIndex() gives the corresponding index in the
          //   internal objective list, where objective 0 is the distance
          //   above hull.
          if (ind >= Objectivei_T) {
            const int dynamicIndex = ind - Objectivei_T;
            double dynamicValue = 0.0;
            if (dynamicIndex < userObjectivesNum) {
              const int objectiveIndex = xtalopt->getUserObjectiveIndex(dynamicIndex);
              if (xtal->getStrucObjState() == Structure::Os_NotCalculated ||
                  objectiveIndex >= xtal->getStrucObjNumber()) {
                usePoint = false;
                continue;
              }
              dynamicValue = xtal->getStrucObjValues(objectiveIndex);
            } else {
              const int constraintIndex = dynamicIndex - userObjectivesNum;
              if (xtal->getStrucConstraintState() == Structure::Cs_NotCalculated ||
                  constraintIndex >= xtal->getStrucConstraintNumber()) {
                usePoint = false;
                continue;
              }
              dynamicValue = xtal->getStrucConstraintValues(constraintIndex);
            }
            switch (j) {
              case 0:
                x = dynamicValue;
                break;
              default:
                y = dynamicValue;
                break;
            }
          }
      }
    }

    if (!usePoint)
      continue;

    QwtPlotMarker* pm = addXtalToPlot(xtal, x, y);

    // See if we should draw another line for the trace
    // This trace assumes the xtals will be ordered based on structure index
    if (performTrace) {
      if (lastTraceStructure == -1) {
        lastTraceStructure = xtal->getIndex();
        minE = y;
      } else if (y < minE) {
        plotTrace(lastTraceStructure, minE, xtal->getIndex(), y);
        lastTraceStructure = xtal->getIndex();
        minE = y;
      } else {
        ui.plot->addHorizontalPlotLine(lastTraceStructure, xtal->getIndex() , minE);
      }
    }

    // Set point label if requested
    QString s;
    if (labelPoints) {
      switch (labelType) {
        case Number_L:
          s = QString::number(xtal->getSpaceGroupNumber());
          break;
        case Symbol_L:
          s = xtal->getSpaceGroupSymbol();
          break;
        case Enthalpy_L:
          if (GS_ISFINITE(xtal->getEnthalpy()))
            s = QString::number(xtal->getEnthalpy(), 'g', 5);
          break;
        case Enthalpy_per_Atm_L:
          if (GS_ISFINITE(xtal->getEnthalpyPerAtom()))
            s = QString::number(xtal->getEnthalpyPerAtom(), 'g', 5);
          break;
        case AboveHull_per_Atm_L:
          if (GS_ISFINITE(xtal->getDistAboveHull()))
            s = QString::number(xtal->getDistAboveHull(), 'g', 5);
          break;
        case ParetoFront_L:
          s = QString::number(xtal->getParetoFront());
          break;
        case AtomCount_L:
          s = QString::number(xtal->numAtoms());
          break;
        case Energy_L:
          if (GS_ISFINITE(xtal->getEnergy()))
            s = QString::number(xtal->getEnergy(), 'g', 5);
          break;
        case PV_L:
          if (GS_ISFINITE(xtal->getPV()))
            s = QString::number(xtal->getPV(), 'g', 5);
          break;
        case Volume_per_Atm_L:
          if (GS_ISFINITE(xtal->getVolumePerAtom()))
            s = QString::number(xtal->getVolumePerAtom(), 'g', 5);
          break;
        case Generation_L:
          s = QString::number(xtal->getGeneration());
          break;
        case StructureINDX_L:
          s = QString::number(xtal->getIndex());
          break;
        case StructureTAG_L:
          s = xtal->getTag();
          break;
        default:
          // Objectives in multi-objective run. Since there is no fixed number of
          //   objectives; and MSVC does not support "case range", we put them
          //   under "default".
          // Their index in the list of options starts from Objectivei_L;
          //   getUserObjectiveIndex() gives the corresponding index in the
          //   internal objective list, where objective 0 is the distance
          //   above hull.
          if (labelType >= Objectivei_L) {
            const int dynamicIndex = labelType - Objectivei_L;
            if (dynamicIndex < userObjectivesNum) {
              const int objectiveIndex = xtalopt->getUserObjectiveIndex(dynamicIndex);
              if (objectiveIndex < xtal->getStrucObjNumber() && GS_ISFINITE(xtal->getStrucObjValues(objectiveIndex))) {
                s = QString::number(xtal->getStrucObjValues(objectiveIndex));
              }
            } else {
              const int constraintIndex = dynamicIndex - userObjectivesNum;
              if (constraintIndex < xtal->getStrucConstraintNumber() && GS_ISFINITE(xtal->getStrucConstraintValues(constraintIndex))) {
                s = QString::number(xtal->getStrucConstraintValues(constraintIndex));
              }
            }
          } else {
            s = xtal->getSpaceGroupSymbol();
          }
        }
      QwtText text(s);
      text.setColor(Qt::black);
      pm->setLabel(text);
      pm->setLabelAlignment(Qt::AlignBottom);
    }
  }

  // Set axis labels
  for (int j = 0; j < 2; j++) { // 0 = x, 1 = y
    switch (j) {
      case 0:
        ind = ui.combo_xAxis->currentIndex();
        break;
      default:
        ind = ui.combo_yAxis->currentIndex();
        break;
    }

    QString label;
    switch (ind) {
      case StructureINDX_T:
        label = tr("Structure Index");
        break;
      case Generation_T:
        label = tr("Generation");
        break;
      case Enthalpy_T:
        label = tr("Enthalpy");
        break;
      case AboveHull_per_Atm_T:
        label = tr("Above Hull per Atom");
        break;
      case ParetoFront_T:
        label = tr("Pareto Front");
        break;
      case AtomCount_T:
        label = tr("Total number of Atoms");
        break;
      case Enthalpy_per_Atm_T: // PSA Enthalpy per atom
        label = tr("Enthalpy per Atom");
        break;
      case Energy_T:
        label = tr("Energy");
        break;
      case PV_T:
        label = tr("Enthalpy PV Term");
        break;
      case A_per_Atm_T:
        label = tr("A per Atom");
        break;
      case B_per_Atm_T:
        label = tr("B per Atom");
        break;
      case C_per_Atm_T:
        label = tr("C per Atom");
        break;
      case Alpha_T:
        label = "<HTML>&alpha;</HTML>";
        break;
      case Beta_T:
        label = "<HTML>&beta;</HTML>";
        break;
      case Gamma_A:
        label = "<HTML>&gamma;</HTML>";
        break;
      case Volume_T:
        label = tr("Volume");
        break;
      case Volume_per_Atm_T:
        label = tr("Volume per Atom");
        break;
      default:
        // Objectives in multi-objective run. Since there is no fixed number of
        //   objectives; and MSVC does not support "case range", we put them
        //   under "default".
        // Their index in the list of options starts from Objectivei_T,
        //   but their "shown" index starts from 1; so we have "ind - Objectivei_T + 1"
        if (ind >= Objectivei_T) {
          const int dynamicIndex = ind - Objectivei_T;
          if (dynamicIndex < userObjectivesNum)
            label = tr("Objective%1").arg(dynamicIndex + 1);
          else
            label = tr("Constraint%1").arg(dynamicIndex - userObjectivesNum + 1);
        }
      }
    if (j == 0)
      ui.plot->setXTitle(label);
    else
      ui.plot->setYTitle(label);
  }

  updatePlotFonts();

  // If the selected xtal still exists, select that one. This function
  // doesn't do anything if nullptr is passed to it.
  ui.plot->selectMarker(m_marker_xtal_map.key(selectedXtal));

  ui.plot->setAutoReplot(true);
  QwtScaleEngine* xScaleEngine = ui.plot->axisScaleEngine(QwtPlot::xBottom);
  if (ui.plot->axisAutoScale(QwtPlot::xBottom)) {
    xScaleEngine->setMargins(0.0, 0.0);
    ui.plot->replot();
    const QwtScaleDiv xScale = ui.plot->axisScaleDiv(QwtPlot::xBottom);
    const double margin = qAbs(xScale.upperBound() - xScale.lowerBound()) * 0.03;
    xScaleEngine->setMargins(margin, margin);
  }
  ui.plot->replot();
  updatePlotLayout();
}

void TabPlot::plotDistHist()
{
  /*
      // Initialize vars
      m_plotObject = new PlotObject(Qt::red, PlotObject::Bars);
      double x, y;
      PlotPoint *pp;
      QList<double> d, f, f_temp;

      // Determine xtal
      int ind = ui.combo_distHistXtal->currentIndex();
      if (ind < 0 || ind > m_search->tracker()->size() - 1) {
        ind = 0;
      }
      Xtal* xtal = qobject_cast<Xtal*>(m_search->tracker()->at(ind));

      // Get histogram
      // If no atoms selected...
      {
        QReadLocker xtalLocker(&xtal->lock());
        xtal->generateIADHistogram(&d, &f, 0, 15, .1);
      }

      // Selected atom histogram section removed along with the old
      // external viewer. A new way of selecting atoms will need to be
      // used to add it back in the future (in case someone wants it)

      // Populate plot object
      for (int i = 0; i < d.size(); i++) {
        x = d.at(i);
        y = f.at(i);
        pp = m_plotObject->addPoint(x,y);
      }

      ui.plot_plot->axis(PlotWidget::BottomAxis)->setLabel(tr("Distance"));
      ui.plot_plot->axis(PlotWidget::LeftAxis)->setLabel(tr("Count"));

      ui.plot_plot->addPlotObject(m_plotObject);

      // Set default limits
      ui.plot_plot->scaleLimits();
      ui.plot_plot->setDefaultLimits(0, 8, 0,
     ui.plot_plot->dataRect().bottom());
  */
}

QwtPlotMarker* TabPlot::addXtalToPlot(Xtal* xtal, double x, double y)
{
  QwtPlotMarker* pm = nullptr;
  const PlotStatusCategory statusCategory = plotStatusCategory(xtal);
  if (statusCategory == Psc_Similar) {
    // Dark Green Square
    pm = ui.plot->addPlotPoint(x, y, QwtSymbol::Rect, QBrush(Qt::darkGreen),
                               QPen(Qt::darkGreen));
  } else if (statusCategory == Psc_Failed) {
    pm = ui.plot->addPlotPoint(x, y, QwtSymbol::DTriangle, QBrush(Qt::red), QPen(Qt::red));
  } else if (statusCategory == Psc_Dismissed) {
    pm = ui.plot->addPlotPoint(x, y, QwtSymbol::Diamond, QBrush(Qt::darkGray),
                               QPen(Qt::darkGray));
  } else if (statusCategory == Psc_Incomplete) {
    QColor orange(255, 140, 0, 255);
    pm = ui.plot->addPlotPoint(x, y, QwtSymbol::Triangle, QBrush(orange), QPen(orange));
  } else {
    // Blue Circle
    pm = ui.plot->addPlotPoint(x, y, QwtSymbol::Ellipse, QBrush(Qt::blue),
                               QPen(Qt::blue));
  }

  m_marker_xtal_map[pm] = xtal;
  return pm;
}

void TabPlot::plotTrace(double x1, double y1, double x2, double y2)
{
  ui.plot->addHorizontalPlotLine(x1, x2, y1);
  ui.plot->addVerticalPlotLine(x2, y1, y2);
}

void TabPlot::plotContextMenu(const QPoint& pos)
{
  Q_UNUSED(pos);

  Xtal* xtal = m_marker_xtal_map.value(ui.plot->selectedMarker(), nullptr);
  if (!xtal)
    return;

  m_context_xtal = xtal;

  QMenu menu;
  QAction* a_clipPOSCAR = menu.addAction("&Copy POSCAR to clipboard");
  menu.addSeparator();
  QAction* a_viewStructure = menu.addAction("View Structure");
  menu.addSeparator();
  QAction* a_plotXrd = menu.addAction("View Simulated XRD Pattern");

  connect(a_clipPOSCAR, &QAction::triggered, this, &TabPlot::clipPOSCARPlot);
  connect(a_viewStructure, &QAction::triggered, this, &TabPlot::viewStructurePlot);
  connect(a_plotXrd, &QAction::triggered, this, &TabPlot::plotXrdPlot);

  QAction* selection = menu.exec(QCursor::pos());
  if (!selection)
    m_context_xtal = nullptr;
}

void TabPlot::clipPOSCARPlot()
{
  if (!m_context_xtal)
    return;

  std::stringstream poscarStream;
  {
    QReadLocker locker(&m_context_xtal->lock());
    Atoms::PoscarFormat::write(*m_context_xtal, poscarStream, m_context_xtal->getLocpath());
  }
  const QString poscar = QString::fromStdString(poscarStream.str());
  if (!poscar.isEmpty()) {
    QApplication::clipboard()->setText(poscar, QClipboard::Clipboard);
    if (QApplication::clipboard()->supportsSelection())
      QApplication::clipboard()->setText(poscar, QClipboard::Selection);
  }

  m_context_xtal = nullptr;
}

void TabPlot::viewStructurePlot()
{
  if (!m_context_xtal)
    return;

  auto* dialog = qobject_cast<XtalOptDialog*>(m_dialog);
  if (dialog)
    dialog->showStructureViewer(m_context_xtal);

  m_context_xtal = nullptr;
}

void TabPlot::plotXrdPlot()
{
  if (!m_context_xtal)
    return;

  auto* dialog = qobject_cast<XtalOptDialog*>(m_dialog);
  if (dialog)
    dialog->showXrdViewer(m_context_xtal);

  m_context_xtal = nullptr;
}

void TabPlot::selectXtal(QwtPlotMarker* pm)
{
  Xtal* xtal = m_marker_xtal_map.value(pm, nullptr);
  if (!xtal)
    return;

  emit selectedGeometryChanged(xtal);
}

void TabPlot::selectGeometryFromIndex(int index)
{
  Structure* selected = nullptr;
  {
    QReadLocker trackerLocker(m_search->tracker()->rwLock());
    const int structureCount = m_search->tracker()->size();
    if (structureCount == 0)
      return;

    if (index < 0 || index >= structureCount)
      index = 0;

    selected = m_search->tracker()->at(index);
  }

  emit selectedGeometryChanged(selected);
}

void TabPlot::highlightXtal(Structure* s)
{
  Xtal* xtal = qobject_cast<Xtal*>(s);

  ui.plot->selectMarker(m_marker_xtal_map.key(xtal));
}
}
