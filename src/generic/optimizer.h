/**********************************************************************
  Optimizer - Generic optimizer interface

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

#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "optbase.h"

#include <QHash>
#include <QObject>
#include <QVariant>
#include <QStringList>

namespace Avogadro {
  class Structure;
  class OptBase;

  class Optimizer : public QObject
  {
    Q_OBJECT

  public:

    explicit Optimizer(OptBase *parent);
    virtual ~Optimizer();

    enum JobState { Unknown = -1, Success, Error, Queued, Running, CommunicationError, Started, Pending};

    virtual void readSettings(const QString &filename = "");
    virtual void writeSettings(const QString &filename = "");

    virtual QString getIDString() {return m_idString;};

    virtual int getNumberOfOptSteps();
    virtual bool writeInputFiles(Structure *structure);
    virtual bool startOptimization(Structure *structure);
    virtual bool getOutputFile(const QString & filename, QStringList & data);
    virtual bool getQueueList(QStringList & queueData);
    virtual bool deleteJob(Structure *structure);
    virtual Optimizer::JobState getStatus(Structure *structure);

    // Checks the queueData list for the jobname (extracted from
    // xtal->fileName + "/job. and sets exists to true if the job name
    // is found. Return value is the job ID.
    virtual int checkIfJobNameExists(Structure *structure, const QStringList &queueData, bool &exists);

    virtual bool update(Structure *structure);
    virtual bool load(Structure *structure);
    virtual bool read(Structure *structure, const QString & filename);

    virtual QString getTemplate(const QString &filename, int optStep);
    virtual QStringList getTemplate(const QString &filename);
    virtual QVariant getData(const QString &identifier);
    QStringList getTemplateNames() {return m_templates.keys();};
    QStringList getDataIdentifiers() {return m_data.keys();};
    QString getUser1() {return m_user1;};
    QString getUser2() {return m_user2;};
    QString getUser3() {return m_user3;};
    QString getUser4() {return m_user4;};

  public slots:
    virtual bool setTemplate(const QString &filename, const QString &templateData, int optStep);
    virtual bool setTemplate(const QString &filename, const QStringList &templateData);
    virtual bool appendTemplate(const QString &filename, const QString &templateData);
    virtual bool removeTemplate(const QString &filename, int optStep);
    virtual bool setData(const QString &identifier, const QVariant &data);
    void setUser1(const QString &s) {m_user1 = s;};
    void setUser2(const QString &s) {m_user2 = s;};
    void setUser3(const QString &s) {m_user3 = s;};
    void setUser4(const QString &s) {m_user4 = s;};

  protected:
    virtual bool writeTemplates(Structure *s);

    virtual void readTemplatesFromSettings(const QString &filename = "");
    virtual void writeTemplatesToSettings(const QString &filename = "");
    virtual void readUserValuesFromSettings(const QString &filename = "");
    virtual void writeUserValuesToSettings(const QString &filename = "");
    virtual void readDataFromSettings(const QString &filename = "");
    virtual void writeDataToSettings(const QString &filename = "");

    virtual bool copyLocalTemplateFilesToRemote(Structure *structure);
    virtual bool copyRemoteToLocalCache(Structure *structure);

    // Template/generic data structures
    QHash<QString, QVariant> m_data;
    QHash<QString, QStringList > m_templates;

    // Filename and string to check for to confirm that optimization
    // completed successfully. Only last 100 lines are checked.
    QString m_completionFilename, m_completionString;

    // List of filenames to check when updating structure (will be
    // checked in order of index)
    QStringList m_outputFilenames;

    // User variables
    QString m_user1, m_user2, m_user3, m_user4;

    // Cached pointer to the optbase
    OptBase *m_opt;

    // Identifier for this optimization type
    QString m_idString;
  };

} // end namespace Avogadro

#endif
