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

#ifndef XTALOPT_XTALOPT_PLOT_H
#define XTALOPT_XTALOPT_PLOT_H

#ifdef _WIN32
#define QWT_DLL
#endif

#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_magnifier.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_panner.h>
#include <qwt_symbol.h>

#include <memory>
#include <cmath>

namespace XtalOpt {

class XtalOptPlot : public QwtPlot
{
  Q_OBJECT
public:
  XtalOptPlot(QWidget* parent = nullptr,
              const QColor& backgroundColor = Qt::white);

  virtual bool eventFilter(QObject* object, QEvent* e) override;

  QwtPlotMarker* addPlotPoint(double x = 0.0, double y = 0.0,
                              QwtSymbol::Style symbol = QwtSymbol::Ellipse,
                              const QBrush& brush = QBrush(Qt::red),
                              const QPen& pen = QPen(Qt::red),
                              const QSize& size = QSize(10, 10));
  QwtPlotMarker* addPlotPoint(const QPointF& p,
                              QwtSymbol::Style symbol = QwtSymbol::Ellipse,
                              const QBrush& brush = QBrush(Qt::red),
                              const QPen& pen = QPen(Qt::red),
                              const QSize& size = QSize(10, 10));

  void addHorizontalPlotLine(double xMin, double xMax, double y);
  void addVerticalPlotLine(double x, double yMin, double yMax);

  void clearPlotMarkers();
  void clearPlotCurves();
  void clearAll();
  void removePlotMarker(size_t i);
  void removePlotMarker(QwtPlotMarker* pm);

  QwtPlotMarker* plotMarker(size_t i);
  std::vector<std::unique_ptr<QwtPlotMarker>>& plotMarkers()
  {
    return m_markerList;
  };

  void setXTitle(const QString& text) { setAxisTitle(xBottom, text); };
  void setYTitle(const QString& text) { setAxisTitle(yLeft, text); };

  QwtPlotMarker* selectedMarker() { return m_selectedMarker; };

  void deselectCurrent();
  void selectMarker(QwtPlotMarker* newMarker);

signals:
  // This signal is emitted when the selected marker changes
  void selectedMarkerChanged(QwtPlotMarker* newMarker);

private:
  virtual void select(const QPoint& pos);

  void highlightMarker(QwtPlotMarker* m);
  void dehighlightMarker(QwtPlotMarker* m);

  // 0 == up
  // 1 == down
  // 2 == left
  // 3 == right
  void shiftMarkerCursor(int direction);

  std::vector<std::unique_ptr<QwtPlotMarker>> m_markerList;
  std::vector<std::unique_ptr<QwtPlotCurve>> m_curveList;
  QwtPlotMarker* m_selectedMarker;
  QwtPlotMagnifier m_magnifier;
  QwtPlotPanner m_panner;
};
}

#endif
