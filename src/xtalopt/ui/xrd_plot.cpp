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

#include "xrd_plot.h"

#include <QApplication>
#include <QDebug>
#include <QMouseEvent>

#include <qwt_plot_canvas.h>
#include <qwt_scale_engine.h>

#include <globalsearch/utilities/makeunique.h>

namespace XtalOpt {

XrdPlot::XrdPlot(QWidget* parent, const QColor& backgroundColor)
  : QwtPlot(parent), m_curveList(std::vector<std::unique_ptr<QwtPlotCurve>>()),
    m_magnifier(canvas()), m_panner(canvas())
{
  resize(600, 600);

  canvas()->installEventFilter(this);

  canvas()->setFocusPolicy(Qt::StrongFocus);
  canvas()->setCursor(Qt::PointingHandCursor);

  setCanvasBackground(backgroundColor);

  setTitle("Simulated Xrd Pattern");

  setXTitle("2*theta (degrees)");
  setYTitle("Intensity");

  axisScaleEngine(QwtPlot::yLeft)->setAttribute(QwtScaleEngine::Floating);
  axisScaleEngine(QwtPlot::yLeft)->setMargins(0.001, 0.001);

  axisScaleEngine(QwtPlot::xBottom)->setAttribute(QwtScaleEngine::Floating);
  axisScaleEngine(QwtPlot::xBottom)->setMargins(0.001, 0.001);

  setAutoReplot(true);
  autoRefresh();
}

void XrdPlot::addHorizontalPlotLine(double xMin, double xMax, double y)
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

void XrdPlot::addVerticalPlotLine(double x, double yMin, double yMax)
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

void XrdPlot::addXrdData(const std::vector<std::pair<double, double>>& data)
{
  auto curve(make_unique<QwtPlotCurve>());
  curve->setStyle(QwtPlotCurve::Lines);
  curve->setPen(QColor("red"), 2.0);

  std::vector<double> xData(data.size());
  std::vector<double> yData(data.size());
  for (size_t i = 0; i < data.size(); ++i) {
    xData[i] = data[i].first;
    yData[i] = data[i].second;
  }

  curve->setSamples(xData.data(), yData.data(), data.size());
  curve->attach(this);

  m_curveList.push_back(std::move(curve));
  autoRefresh();
}

void XrdPlot::clearPlotCurves()
{
  for (size_t i = 0; i < m_curveList.size(); ++i)
    m_curveList[i]->detach();
  m_curveList.clear();
}

void XrdPlot::removePlotCurve(size_t i)
{
  if (i >= m_curveList.size()) {
    qDebug() << "Error: removePlotCurve() was called with an invalid index!";
    return;
  }

  m_curveList[i]->detach();
  m_curveList.erase(m_curveList.begin() + i);
}

void XrdPlot::removePlotCurve(QwtPlotCurve* pc)
{
  for (size_t i = 0; i < m_curveList.size(); ++i) {
    if (pc == m_curveList[i].get()) {
      removePlotCurve(i);
      return;
    }
  }
  qDebug() << "Error: curve in parameter not found in removePlotCurve()!";
}

QwtPlotCurve* XrdPlot::plotCurve(size_t i)
{
  if (i >= m_curveList.size()) {
    qDebug() << "Error: plotCurve() was called with an invalid index!";
    return nullptr;
  }
  return m_curveList[i].get();
}

bool XrdPlot::eventFilter(QObject* object, QEvent* e)
{
  switch (e->type()) {
    case QEvent::MouseButtonPress: {
      // We'll keep this going so we can pan also
      return false;
    }
    default:
      break;
  }
  return QObject::eventFilter(object, e);
}
}
