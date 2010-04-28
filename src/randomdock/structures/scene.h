/**********************************************************************
  RandomDock - Scene: Wrapper for Avogadro::Molecule to hold the 
  central molecule and matrix elements in a docking problem

  Copyright (C) 2009 by David C. Lonie

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

#ifndef SCENEMOL_H
#define SCENEMOL_H

#include <avogadro/molecule.h>

#include <QTextStream>

namespace Avogadro {
  class RandomDockParams;

  class Scene : public Molecule
  {
    Q_OBJECT

   public:
    Scene(QObject *parent = 0);
    virtual ~Scene();

    enum State {Optimized = 0, WaitingForOptimization, InProcess, Empty, Updating, Error, Submitted, Killed};

    QString getRempath() {return m_rempath;};
    int getSceneNumber() {return m_sceneNumber;};
    bool isOptimized() {if (m_status == Optimized) return true; else return false;};
    double getEnergy() {return energy(0);};
    int getEnergyRank() {return m_rank;};
    QReadWriteLock* rwLock() {return m_rwLock;};
    State getStatus() {return m_status;};
    int getJobID() {return m_jobID;};

   signals:

   public slots:
    void setRempath(const QString & s) {m_rempath = s;};
    void setSceneNumber(int i) {m_sceneNumber = i;};
    void setEnergyRank(int r) {m_rank = r;};
    void setStatus(State s) {m_status = s;};
    void setJobID(int j) {m_jobID = j;};
    void updateFromMolecule(Molecule *mol);

   private slots:

   private:
    QString m_rempath;
    int m_sceneNumber;
    State m_status;
    int m_rank;
    int m_jobID;
    QReadWriteLock *m_rwLock;
  };

} // end namespace Avogadro

#endif
