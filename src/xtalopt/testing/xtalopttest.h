/**********************************************************************
  XtalOptTest - Automagically generate a ton of data from multiple runs

  Copyright (C) 2009-2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef XTALOPTTEST_H
#define XTALOPTTEST_H

#include <QDateTime>
#include <QObject>

class QProgressDialog;

namespace XtalOpt {

class XtalOpt;
class XtalOptDialog;

class XtalOptTest : public QObject
{
  Q_OBJECT

public:
  XtalOptTest(XtalOpt* p, QObject* parent = 0);
  virtual ~XtalOptTest() override;

  void start();
  void gatherData();
  void showDialog();
  void generateRun(int run);
  void writeDataFile(int run);
  void resetOpt();
  bool isFinished();
  int getCurrentStructure();

signals:
  void testStarting();
  void newMessage(const QString&);
  void status();
  void sig_updateProgressDialog();

public slots:
  void updateMessage(const QString&);
  void updateStatus();
  void updateProgressDialog();
  void outputStatus(const QString&, int, int, int, int, int, int);

private slots:

private:
  QProgressDialog* m_prog;
  int m_currentRun, m_numberRuns, m_startRun, m_endRun, m_currentStructure,
    m_numberStructures, m_totalNumberStructures;
  QString m_message;
  QDateTime m_begin;
  XtalOpt* m_opt;
  XtalOptDialog* m_dialog;
};
} // end namespace XtalOpt

#endif
