/**********************************************************************
  OptBase - Base class for global search extensions

  Copyright (C) 2010 by David C. Lonie

  This file is part of the Avogadro molecular editor project.
  For more information, see <http://avogadro.openmolecules.net/>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifndef OPTBASE_H
#define OPTBASE_H

#include <QObject>
#include <QMutex>

namespace Avogadro {
  class Structure;
  class Tracker;
  class Optimizer;
  class QueueManager;

  class OptBase : public QObject
  {
    Q_OBJECT

   public:
    explicit OptBase(QObject *parent);
    virtual ~OptBase();

    enum FailActions {
      FA_DoNothing = 0,
      FA_KillIt,
      FA_Randomize
    };

    virtual Structure* replaceWithRandom(Structure *s, const QString & reason) {};

    virtual bool checkLimits() {};

    virtual bool save() {};
    virtual bool load(const QString & filename) {};

    Tracker* tracker(){return m_tracker;};
    QueueManager* queue(){return m_queue;};
    Optimizer* optimizer() {return m_optimizer;};

    bool limitRunningJobs;		// Whether to impose the running job limit
    uint runningJobLimit;		// Number of concurrent jobs allowed.
    uint contStructs;                   // Number of continuous structures generated

    bool testingMode;			// Whether we are running tests
    uint test_nRunsStart;		// Starting run number
    uint test_nRunsEnd;			// Ending run number
    uint test_nStructs;			// Number of structures per run when testing

    uint failLimit;			// Number of times a structure may fail
    FailActions failAction;		// What to do on excessive failures

    // sOBMutex is here because OB likes to implement singleton
    // classes that aren't thread safe.
    QMutex *sOBMutex, *stateFileMutex, *backTraceMutex;

    bool savePending, isStarting;

   signals:
    void startingSession();
    void sessionStarted();
    void optimizerChanged(Optimizer*);
    void debugStatement(const QString &);
    void warningStatement(const QString &);
    void errorStatement(const QString &);

   public slots:
    virtual void reset();
    virtual void startOptimization() {};
    virtual void generateNewStructure() {};
    void warning(const QString & s);
    void debug(const QString & s);
    void error(const QString & s);
    void emitSessionStarted() {emit sessionStarted();};
    void emitStartingSession() {emit startingSession();};
    void setIsStartingTrue() {isStarting = true;};
    void setIsStartingFalse() {isStarting = false;};
    void printBackTrace();
    void setOptimizer(Optimizer *o) {setOptimizer_opt(o);};
    void setOptimizer(const QString &IDString) {setOptimizer_string(IDString);};

   protected:
    Tracker *m_tracker;
    QueueManager *m_queue;
    Optimizer *m_optimizer;

    virtual void setOptimizer_opt(Optimizer *o);
    virtual void setOptimizer_string(const QString &s) {};
  };

} // end namespace Avogadro

#endif
