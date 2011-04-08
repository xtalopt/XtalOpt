/**********************************************************************
  RandomDock - Matrix: Wrapper for Avogadro::Molecule to hold the
  matrix monomers in a docking problem

  Copyright (C) 2009-2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifndef MATRIXMOL_H
#define MATRIXMOL_H

#include <globalsearch/structure.h>

#include <QtCore/QDateTime>
#include <QtCore/QTextStream>

namespace RandomDock {

  class Matrix : public GlobalSearch::Structure
  {
    Q_OBJECT

   public:
    Matrix(QObject *parent = 0);
    Matrix(Avogadro::Molecule *mol);
    virtual ~Matrix();

   signals:

   public slots:
    double prob(uint index) { checkProbs(); return m_probs.at(index);};
    void sortConformers();
    void generateProbabilities();
    void checkProbs() {if ((uint)m_probs.size() != numConformers()) generateProbabilities();};
    int getRandomConformerIndex();

   private slots:

   private:
    QList<double> m_probs;
  };

} // end namespace RandomDock

#endif
