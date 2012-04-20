/**********************************************************************
  XtalOptTest - Automagically generate a ton of data from multiple runs

  Copyright (C) 2009-2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include "xtalopttest.h"

#include <xtalopt/xtalopt.h>
#include <xtalopt/ui/dialog.h>

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QTimer>
#include <QtCore/QtConcurrentRun>

#include <QtGui/QInputDialog>
#include <QtGui/QProgressDialog>

using namespace GlobalSearch;

namespace XtalOpt {

  XtalOptTest::XtalOptTest(XtalOpt *p, QObject *parent) :
    QObject(parent),
    m_opt(p),
    m_dialog(qobject_cast<XtalOptDialog*>(p->dialog()))
  {
    connect(this, SIGNAL(testStarting()),
            m_dialog, SLOT(lockGUI()),
            Qt::BlockingQueuedConnection);
    connect(this, SIGNAL(testStarting()),
            m_dialog, SLOT(disconnectGUI()),
            Qt::BlockingQueuedConnection);
  }

  XtalOptTest::~XtalOptTest()
  {
    m_prog->deleteLater();
  }


  void XtalOptTest::start()
  {
    emit testStarting();

    // Prompt user for number of runs and structures
    gatherData();

    // Display progress dialog
    showDialog();

    // Start timer
    m_begin = QDateTime::currentDateTime();

    // Initialize dialog values
    emit newMessage("Initializing test routine...");
    emit status();

    // Start optimization
    emit newMessage("Running tests...");
    for (int run = m_startRun; run <= m_endRun; run++) {
      generateRun(run);
      writeDataFile(run);
    }
  }

  void XtalOptTest::gatherData() {
    m_numberStructures = m_opt->test_nStructs;
    m_startRun = m_opt->test_nRunsStart;
    m_endRun = m_opt->test_nRunsEnd;
    m_numberRuns = m_endRun - m_startRun + 1;
    m_totalNumberStructures = m_numberStructures * m_numberRuns;
  }

  void XtalOptTest::showDialog() {
    connect(this, SIGNAL(newMessage(const QString &)),
            this, SLOT(updateMessage(const QString &)));
    connect(this, SIGNAL(status()),
            this, SLOT(updateStatus()));
    connect(this, SIGNAL(sig_updateProgressDialog()),
            this, SLOT(updateProgressDialog()));
  }

  void XtalOptTest::generateRun(int run) {
    int m_currentStructure = 0;
    int m_currentRun = run;
    emit sig_updateProgressDialog();
    // Perform run
    emit status();
    resetOpt();
    m_opt->startSearch();
    emit status();
    m_message = "Looping...";
    while (m_opt->tracker()->size() < m_numberStructures) {
      //m_opt->queue()->checkPopulation();
      //m_opt->queue()->checkRunning();
      m_currentStructure = getCurrentStructure();
      outputStatus(m_message,
                   m_currentRun - m_startRun + 1,
                   m_numberRuns,
                   m_currentStructure,
                   m_numberStructures,
                   (m_currentRun-m_startRun) * m_numberStructures + m_currentStructure,
                   m_totalNumberStructures);
#ifdef WIN32
      _sleep(1000);
#else
      sleep(1);
#endif // _WIN32
    }
    m_message = "Waiting to finish...";
    while (!isFinished()) {
      //m_opt->queue()->checkPopulation();
      //m_opt->queue()->checkRunning();
      m_currentStructure = getCurrentStructure();
      outputStatus(m_message,
                   m_currentRun - m_startRun + 1,
                   m_numberRuns,
                   m_currentStructure,
                   m_numberStructures,
                   (m_currentRun-m_startRun) * m_numberStructures + m_currentStructure,
                   m_totalNumberStructures);
#ifdef WIN32
      _sleep(1000);
#else
      sleep(1);
#endif // _WIN32
    }
  }

  bool XtalOptTest::isFinished() {
    int done = 0;
    m_opt->tracker()->lockForRead();
    QList<Structure*> *structures = m_opt->tracker()->list();
    Xtal* xtal = 0;
    for (int i = 0; i < structures->size(); i++) {
      xtal = qobject_cast<Xtal*>(structures->at(i));
      xtal->lock()->lockForRead();
      Xtal::State state = xtal->getStatus();
      xtal->lock()->unlock();
      if (state == Xtal::Optimized ||
          state == Xtal::Killed ||
          state == Xtal::Duplicate ||
          state == Xtal::Removed)
        done++;
    }
    m_opt->tracker()->unlock();
    if (done >= m_numberStructures) return true;
    else return false;
  }

  int XtalOptTest::getCurrentStructure() {
    int n = 0;
    m_opt->tracker()->lockForRead();
    QList<Structure*> *structures = m_opt->tracker()->list();
    Xtal* xtal = 0;
    for (int i = 0; i < structures->size(); i++) {
      xtal = qobject_cast<Xtal*>(structures->at(i));
      xtal->lock()->lockForRead();
      Xtal::State state = xtal->getStatus();
      xtal->lock()->unlock();
      if (state == Xtal::InProcess ||
          state == Xtal::Optimized ||
          state == Xtal::Submitted ||
          state == Xtal::Killed ||
          state == Xtal::Restart ||
          state == Xtal::Duplicate ||
          state == Xtal::Removed)
        n++;
    }
    m_opt->tracker()->unlock();
    return n;
  }

  void XtalOptTest::resetOpt() {
    m_opt->reset();
  }

  void XtalOptTest::writeDataFile(int run) {
    qDebug() << "Run " << run << " Finished!!" << endl;
    QFile file;
    file.setFileName(m_opt->filePath + "/run" +
                     QString::number(run) + "-results.txt");
    if (!file.open(QIODevice::WriteOnly)) {
      m_opt->error("XtalOptTest::writeDataFile(): Error opening file "+file.fileName()+" for writing...");
    }
    QTextStream out;
    out.setDevice(&file);
    m_opt->tracker()->lockForRead();
    QList<Structure*> *structures = m_opt->tracker()->list();
    Xtal *xtal;

    // Print the data to the file:
    out << "Index\tGen\tID\tEnthalpy\tSpaceGroup\tStatus\tParentage\n";
    for (int i = 0; i < structures->size(); i++) {
      xtal = qobject_cast<Xtal*>(structures->at(i));
      if (!xtal) continue; // In case there was a problem copying.
      xtal->lock()->lockForRead();
      out << i << "\t"
          << xtal->getGeneration() << "\t"
          << xtal->getIDNumber() << "\t"
          << xtal->getEnthalpy() << "\t\t"
          << xtal->getSpaceGroupNumber() << ": " << xtal->getSpaceGroupSymbol() << "\t\t";
      // Status:
      switch (xtal->getStatus()) {
      case Xtal::Optimized:
        out << "Optimized";
        break;
      case Xtal::Killed:
      case Xtal::Removed:
        out << "Killed";
        break;
      case Xtal::Duplicate:
        out << "Duplicate";
        break;
      case Xtal::Error:
        out << "Error";
        break;
      case Xtal::Preoptimizing:
        out << "Preoptimizing";
        break;
      case Xtal::StepOptimized:
      case Xtal::WaitingForOptimization:
      case Xtal::InProcess:
      case Xtal::Empty:
      case Xtal::Updating:
      case Xtal::Submitted:
      case Xtal::Restart:
      default:
        out << "In progress";
        break;
      }
      // Parentage:
      out << "\t" << xtal->getParents();
      xtal->lock()->unlock();
      out << endl;
    }
    m_opt->tracker()->unlock();
  }

  void XtalOptTest::updateMessage(const QString & text) {
    m_message = text;
    emit sig_updateProgressDialog();
  }

  void XtalOptTest::updateStatus() {
    emit sig_updateProgressDialog();
  }

  void XtalOptTest::updateProgressDialog() {
  }

  void XtalOptTest::outputStatus(const QString & message, int currentRun, int numberRuns, int currentStructure, int numberStructures, int totalStructures, int totalNumberStructures) {
    int secondsElapsed = -QDateTime::currentDateTime().secsTo(m_begin);
    double secondsPerStructure = secondsElapsed/double(totalStructures);
    int remaining = int((totalNumberStructures - totalStructures) * secondsPerStructure);
    int h = int(remaining / 3600);
    remaining -= h*3600;
    int m = int(remaining / 60);
    remaining -= m*60;
    int s = remaining;
    QString remainingTime = QString("%1:%2:%3")
      .arg(QString::number(h), 2, '0')
      .arg(QString::number(m), 2, '0')
      .arg(QString::number(s), 2, '0');
    QString finishedAt = m_begin.addSecs(int(totalNumberStructures * secondsPerStructure)).toString("ddd, MMM dd hh:mm:ss");

    qDebug() << "";
    for (int i = 0; i < 2; i++) {
      qDebug() << "-----------------------------------------------------------------------------------";
    }
    for (int i = 0; i < 2; i++) {
      qDebug() << "###################################################################################";
    }
    qDebug() << tr("%1\nCurrent run: %2 of %3\nCurrent Structure: %4 of %5\nTotal Structures: %6 of %7")
      .arg(message)
      .arg(currentRun)
      .arg(numberRuns)
      .arg(currentStructure)
      .arg(numberStructures)
      .arg(totalStructures)
      .arg(totalNumberStructures);
    qDebug() << tr("%1 remaining, estimating finish at %2\ngiven the current rate of %3 seconds per structure.")
      .arg(remainingTime).arg(finishedAt).arg(QString::number(secondsPerStructure));
    for (int i = 0; i < 2; i++) {
      qDebug() << "###################################################################################";
    }
    for (int i = 0; i < 2; i++) {
      qDebug() << "-----------------------------------------------------------------------------------";
    }
    qDebug() << "";
  }

} // end namespace XtalOpt
