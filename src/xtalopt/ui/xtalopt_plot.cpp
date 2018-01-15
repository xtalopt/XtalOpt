/**********************************************************************
  XtalOptPlot - a plot class for analyzing the XtalOpt run

  Copyright (C) 2017-2018 Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include "xtalopt_plot.h"

#include <QApplication>
#include <QDebug>
#include <QMouseEvent>

#include <qwt_plot_canvas.h>

#include <globalsearch/utilities/makeunique.h>

namespace XtalOpt {

XtalOptPlot::XtalOptPlot(QWidget* parent, const QColor& backgroundColor)
  : QwtPlot(parent),
    m_markerList(std::vector<std::unique_ptr<QwtPlotMarker>>()),
    m_curveList(std::vector<std::unique_ptr<QwtPlotCurve>>()),
    m_selectedMarker(nullptr), m_magnifier(canvas()), m_panner(canvas())
{
  canvas()->installEventFilter(this);

  canvas()->setFocusPolicy(Qt::StrongFocus);
  canvas()->setCursor(Qt::PointingHandCursor);

  setCanvasBackground(backgroundColor);
  replot();
}

QwtPlotMarker* XtalOptPlot::addPlotPoint(double x, double y,
                                         QwtSymbol::Style symbol,
                                         const QBrush& brush, const QPen& pen,
                                         const QSize& size)
{
  return addPlotPoint(QPointF(x, y), symbol, brush, pen, size);
}

QwtPlotMarker* XtalOptPlot::addPlotPoint(const QPointF& p,
                                         QwtSymbol::Style symbol,
                                         const QBrush& brush, const QPen& pen,
                                         const QSize& size)
{
  auto plotMarker(make_unique<QwtPlotMarker>());
  plotMarker->setSymbol(new QwtSymbol(symbol, brush, pen, size));
  plotMarker->setValue(p);
  plotMarker->setItemAttribute(QwtPlotItem::AutoScale, true);
  plotMarker->attach(this);
  m_markerList.push_back(std::move(plotMarker));
  autoRefresh();
  return m_markerList[m_markerList.size() - 1].get();
}

void XtalOptPlot::addHorizontalPlotLine(double xMin, double xMax, double y)
{
  auto curve(make_unique<QwtPlotCurve>());
  curve->setStyle(QwtPlotCurve::Lines);

  double xData[2];
  double yData[2];
  xData[0] = xMin;
  xData[1] = xMax;
  yData[0] = y;
  yData[1] = y;

  curve->setSamples(xData, yData, 2);
  curve->attach(this);

  m_curveList.push_back(std::move(curve));
  autoRefresh();
}

void XtalOptPlot::addVerticalPlotLine(double x, double yMin, double yMax)
{
  auto curve(make_unique<QwtPlotCurve>());
  curve->setStyle(QwtPlotCurve::Lines);

  double xData[2];
  double yData[2];
  xData[0] = x;
  xData[1] = x;
  yData[0] = yMin;
  yData[1] = yMax;

  curve->setSamples(xData, yData, 2);
  curve->attach(this);

  m_curveList.push_back(std::move(curve));
  autoRefresh();
}

void XtalOptPlot::clearPlotMarkers()
{
  deselectCurrent();
  for (size_t i = 0; i < m_markerList.size(); ++i)
    m_markerList[i]->detach();
  m_markerList.clear();
}

void XtalOptPlot::clearPlotCurves()
{
  for (size_t i = 0; i < m_curveList.size(); ++i)
    m_curveList[i]->detach();
  m_curveList.clear();
}

void XtalOptPlot::clearAll()
{
  clearPlotMarkers();
  clearPlotCurves();
}

void XtalOptPlot::removePlotMarker(size_t i)
{
  if (i >= m_markerList.size()) {
    qDebug() << "Error: removePlotMarker() was called with an invalid index!";
    return;
  }

  if (m_markerList[i].get() == m_selectedMarker)
    deselectCurrent();

  m_markerList[i]->detach();
  m_markerList.erase(m_markerList.begin() + i);
}

void XtalOptPlot::removePlotMarker(QwtPlotMarker* pm)
{
  for (size_t i = 0; i < m_markerList.size(); ++i) {
    if (pm == m_markerList[i].get()) {
      removePlotMarker(i);
      return;
    }
  }
  qDebug() << "Error: marker in parameter not found in removePlotMarker()!";
}

QwtPlotMarker* XtalOptPlot::plotMarker(size_t i)
{
  if (i >= m_markerList.size()) {
    qDebug() << "Error: plotMarker() was called with an invalid index!";
    return nullptr;
  }
  return m_markerList[i].get();
}

bool XtalOptPlot::eventFilter(QObject* object, QEvent* e)
{
  switch (e->type()) {
    case QEvent::MouseButtonPress: {
      select(((QMouseEvent*)e)->pos());
      // We'll keep this going so we can pan also
      return false;
    }
    case QEvent::KeyPress: {
      switch (((const QKeyEvent*)e)->key()) {
        case Qt::Key_Up:
          shiftMarkerCursor(0);
          return true;

        case Qt::Key_Down:
          shiftMarkerCursor(1);
          return true;

        case Qt::Key_Left:
          shiftMarkerCursor(2);
          return true;

        case Qt::Key_Right:
          shiftMarkerCursor(3);
          return true;
      }
    }
    default:
      break;
  }
  return QObject::eventFilter(object, e);
}

// Find the distance between two QPointF's
static inline double distance(const QPointF& p1, const QPointF& p2)
{
  return sqrt(pow(p1.x() - p2.x(), 2.0) + pow(p1.y() - p2.y(), 2.0));
}

// Select the point at a position.
void XtalOptPlot::select(const QPoint& pos)
{
  QwtPlotMarker* selection = nullptr;

  // Must be within 10 pixels at least
  double dist = 10.0;

  const QwtPlotItemList& itmList = itemList();
  for (QwtPlotItemIterator it = itmList.begin(); it != itmList.end(); ++it) {
    if ((*it)->rtti() == QwtPlotItem::Rtti_PlotMarker) {
      QwtPlotMarker* m = static_cast<QwtPlotMarker*>(*it);
      // We have to map the plot coordinates onto widget coordinates before
      // comparing
      const QwtScaleMap& xMap = canvasMap(m->xAxis());
      const QwtScaleMap& yMap = canvasMap(m->yAxis());
      double mx = xMap.transform(m->xValue());
      double my = yMap.transform(m->yValue());
      double d = distance(QPointF(mx, my), pos);
      if (d < dist) {
        selection = m;
        dist = d;
      }
    }
  }

  selectMarker(selection);
}

void XtalOptPlot::highlightMarker(QwtPlotMarker* m)
{
  // Copy over everything from the whole symbol except the brush color
  QwtSymbol* sym = new QwtSymbol(m->symbol()->style(), QBrush(Qt::white),
                                 m->symbol()->pen(), m->symbol()->size());
  m->setSymbol(sym);
}

void XtalOptPlot::dehighlightMarker(QwtPlotMarker* m)
{
  // Copy over everything from the whole symbol except the brush color
  QwtSymbol* sym =
    new QwtSymbol(m->symbol()->style(), QBrush(m->symbol()->pen().color()),
                  m->symbol()->pen(), m->symbol()->size());
  m->setSymbol(sym);
}

void XtalOptPlot::deselectCurrent()
{
  if (!m_selectedMarker)
    return;

  dehighlightMarker(m_selectedMarker);
  m_selectedMarker = nullptr;
  emit selectedMarkerChanged(m_selectedMarker);
  autoRefresh();
}

void XtalOptPlot::selectMarker(QwtPlotMarker* newMarker)
{
  if (!newMarker)
    return;

  // If we are selecting the point we already have, just return
  if (newMarker == m_selectedMarker)
    return;

  deselectCurrent();
  highlightMarker(newMarker);
  m_selectedMarker = newMarker;
  emit selectedMarkerChanged(m_selectedMarker);
  autoRefresh();
}

// Select the next/previous neighbour of the selected point
// 0 = up
// 1 = down
// 2 = left
// 3 = right
void XtalOptPlot::shiftMarkerCursor(int direction)
{
  if (!m_selectedMarker)
    return;

  QwtPlotMarker* selection = nullptr;

  double dist = 1e300;

  const QwtPlotItemList& itmList = itemList();
  for (QwtPlotItemIterator it = itmList.begin(); it != itmList.end(); ++it) {
    if ((*it)->rtti() == QwtPlotItem::Rtti_PlotMarker) {
      QwtPlotMarker* m = static_cast<QwtPlotMarker*>(*it);

      // Skip over the point that is already selected
      if (m == m_selectedMarker)
        continue;

      // Find the distance and see if it is the shortest so far

      // If we ever want to do this with canvas coordinates, this is how
      // it is done:

      // double x1 = canvasMap(m->xAxis()).transform(m->xValue());
      // double y1 = canvasMap(m->yAxis()).transform(m->yValue());
      // double x2 = canvasMap(m_selectedMarker->xAxis())
      //                 .transform(m_selectedMarker->xValue());
      // double y2 = canvasMap(m_selectedMarker->yAxis())
      //                 .transform(m_selectedMarker->yValue());
      // double d = distance(QPointF(x1, y1), QPointF(x2, y2));

      double d = distance(m->value(), m_selectedMarker->value());
      if (d > dist)
        continue;

      // Let's make sure the direction is correct as well
      switch (direction) {
        // up
        case 0: {
          // Check to make sure this is actually up
          if (m->yValue() - m_selectedMarker->yValue() > 0.0) {
            selection = m;
            dist = d;
          }
          break;
        }
        // down
        case 1: {
          // Check to make sure this is actually down
          if (m_selectedMarker->yValue() - m->yValue() > 0.0) {
            selection = m;
            dist = d;
          }
          break;
        }
        // left
        case 2: {
          // Check to make sure this is actually left
          if (m_selectedMarker->xValue() - m->xValue() > 0.0) {
            selection = m;
            dist = d;
          }
          break;
        }
        // right
        case 3: {
          // Check to make sure this is actually right
          if (m->xValue() - m_selectedMarker->xValue() > 0.0) {
            selection = m;
            dist = d;
          }
          break;
        }
      }
    }
  }

  selectMarker(selection);
}
}
