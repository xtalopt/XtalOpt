/**********************************************************************
  XrdPlot - a plot class for generating an Xrd pattern

  Copyright (C) 2017-2018 Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef XTALOPT_XRD_PLOT_H
#define XTALOPT_XRD_PLOT_H

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
#include <utility>
#include <vector>

namespace XtalOpt {

class XrdPlot : public QwtPlot
{
  Q_OBJECT
public:
  XrdPlot(QWidget* parent = nullptr, const QColor& backgroundColor = Qt::white);

  virtual bool eventFilter(QObject* object, QEvent* e) override;

  void addHorizontalPlotLine(double xMin, double xMax, double y);
  void addVerticalPlotLine(double x, double yMin, double yMax);

  void addXrdData(const std::vector<std::pair<double, double>>& data);

  void clearPlotCurves();

  void removePlotCurve(QwtPlotCurve* pc);
  void removePlotCurve(size_t i);

  QwtPlotCurve* plotCurve(size_t i);
  std::vector<std::unique_ptr<QwtPlotCurve>>& plotCurves()
  {
    return m_curveList;
  };

  void setXTitle(const QString& text) { setAxisTitle(xBottom, text); };
  void setYTitle(const QString& text) { setAxisTitle(yLeft, text); };

private:
  std::vector<std::unique_ptr<QwtPlotCurve>> m_curveList;
  QwtPlotMagnifier m_magnifier;
  QwtPlotPanner m_panner;
};
}

#endif
