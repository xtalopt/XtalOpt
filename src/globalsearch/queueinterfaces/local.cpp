/**********************************************************************
  LocalQueueInterface - Interface for running jobs locally.

  Copyright (C) 2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include <globalsearch/queueinterfaces/local.h>

#include <globalsearch/macros.h>
#include <globalsearch/optimizer.h>
#include <globalsearch/queuemanager.h>
#include <globalsearch/structure.h>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QHash>
#include <QtCore/QProcess>
#include <QtCore/QString>
#include <QtCore/QTextStream>

namespace GlobalSearch {

  LocalQueueInterface::LocalQueueInterface(OptBase *parent,
                                           const QString &settingFile) :
    QueueInterface(parent)
  {
    m_idString = "Local";
    m_hasDialog = false;
  }

  LocalQueueInterface::~LocalQueueInterface()
  {
  }

 bool LocalQueueInterface::writeFiles
  (Structure *s, const QHash<QString, QString> &fileHash) const
  {
    // Create file objects
    QList<QFile*> files;
    QStringList filenames = fileHash.keys();
    for (int i = 0; i < filenames.size(); i++) {
      files.append(new QFile (s->fileName() + "/" + filenames.at(i)));
    }

    // Check that the files can be written to
    for (int i = 0; i < files.size(); i++) {
      if (!files.at(i)->open( QIODevice::WriteOnly | QIODevice::Text ) ) {
        m_opt->error(tr("Cannot write input file %1 (file writing failure)", "1 is a file path").arg(files.at(i)->fileName()));
        qDeleteAll(files);
        return false;
      }
    }

    // Set up text streams
    QList<QTextStream*> streams;
    for (int i = 0; i < files.size(); i++) {
      streams.append(new QTextStream (files.at(i)));
    }

    // Write files
    for (int i = 0; i < streams.size(); i++) {
      *(streams.at(i)) << fileHash[filenames.at(i)];
    }

    // Close files
    for (int i = 0; i < files.size(); i++) {
      files.at(i)->close();
    }

    // Clean up
    qDeleteAll(streams);
    qDeleteAll(files);
    return true;
  }

  bool LocalQueueInterface::prepareForStructureUpdate(Structure *s) const
  {
    // Nothing to do!
    return true;
  }

  bool LocalQueueInterface::checkIfFileExists(Structure *s,
                                              const QString &filename,
                                              bool *exists)
  {
    *exists = QFile::exists(QString("%1/%2")
                            .arg(s->fileName())
                            .arg(filename));
    return true;
  }

  bool LocalQueueInterface::fetchFile(Structure *s,
                                      const QString &rel_filename,
                                      QString *contents) const
  {
    QString filename = s->fileName() + "/" + rel_filename;
    QFile output (filename);
    if (!output.open(QFile::ReadOnly | QFile::Text)) {
      return false;
    }
    *contents = QString(output.readAll());
    output.close();
    return true;
  }

  bool LocalQueueInterface::grepFile(Structure *s,
                                     const QString &matchText,
                                     const QString &filename,
                                     QStringList *matches,
                                     int *exitcode,
                                     const bool caseSensitive) const
  {
    if (exitcode) {
      *exitcode = 1;
    }
    QStringList list;
    QString contents;
    if (!fetchFile(s, filename, &contents)) {
      if (exitcode) {
        *exitcode = 2;
      }
      return false;
    }

    list = contents.split("\n", QString::SkipEmptyParts);

    Qt::CaseSensitivity cs;
    if (caseSensitive) {
      cs = Qt::CaseSensitive;
    }
    else {
      cs = Qt::CaseInsensitive;
    }

    for (QStringList::const_iterator
           it = list.begin(),
           it_end = list.end();
         it != it_end;
         ++it) {
      if ((*it).contains(matchText, cs)) {
        if (exitcode) {
          *exitcode = 0;
        }
        if (matches) {
          *matches << *it;
        }
      }
    }

    return true;
  }
}
