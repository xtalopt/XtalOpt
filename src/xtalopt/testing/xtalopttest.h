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

#ifndef XTALOPTTEST_H
#define XTALOPTTEST_H

#include <xtalopt/xtalopt.h>

#include <QtCore/QDebug>

#include <QtGui/QProgressDialog>

namespace XtalOpt {

  class XtalOptTest : public QObject
  {
    Q_OBJECT

   public:
    XtalOptTest(XtalOpt *p, QObject *parent = 0);
    virtual ~XtalOptTest();

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
    void newMessage(const QString &);
    void status();
    void sig_updateProgressDialog();

   public slots:
    void updateMessage(const QString &);
    void updateStatus();
    void updateProgressDialog();
    void outputStatus(const QString&,int,int,int,int,int,int);

   private slots:

   private:
    QProgressDialog *m_prog;
    int m_currentRun, m_numberRuns, m_startRun, m_endRun, m_currentStructure, m_numberStructures, m_totalNumberStructures;
    QString m_message;
    QDateTime m_begin;
    XtalOpt *m_opt;
    XtalOptDialog *m_dialog;
  };
} // end namespace XtalOpt

#endif
