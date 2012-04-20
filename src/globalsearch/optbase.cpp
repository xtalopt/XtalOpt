/**********************************************************************
  OptBase - Base class for global search extensions

  Copyright (C) 2010-2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include <globalsearch/optbase.h>

#include <globalsearch/bt.h>
#include <globalsearch/macros.h>
#include <globalsearch/optimizer.h>
#include <globalsearch/queuemanager.h>
#include <globalsearch/queueinterface.h>
#include <globalsearch/queueinterfaces/local.h>
#include <globalsearch/queueinterfaces/pbs.h>
#ifdef ENABLE_SSH
#include <globalsearch/sshmanager.h>
#ifdef USE_CLI_SSH
#include <globalsearch/sshmanager_cli.h>
#else // USE_CLI_SSH
#include <globalsearch/sshmanager_libssh.h>
#endif // USE_CLI_SSH
#endif // ENABLE_SSH
#include <globalsearch/structure.h>
#include <globalsearch/ui/abstractdialog.h>

#include <QtCore/QFile>
#include <QtCore/QThread>

#include <QtGui/QClipboard>
#include <QtGui/QMessageBox>
#include <QtGui/QApplication>
#include <QtGui/QInputDialog>

using namespace OpenBabel;

namespace GlobalSearch {

  OptBase::OptBase(AbstractDialog *parent) :
    QObject(parent),
    m_dialog(parent),
    m_tracker(new Tracker (this)),
    m_queueThread(new QThread),
    m_queue(new QueueManager(m_queueThread, this)),
    m_queueInterface(0), // This will be set when the GUI is initialized
    m_optimizer(0),      // This will be set when the GUI is initialized
#ifdef ENABLE_SSH
    m_ssh(NULL),
#endif // ENABLE_SSH
    m_idString("Generic"),
    sOBMutex(new QMutex),
    stateFileMutex(new QMutex),
    backTraceMutex(new QMutex),
    usePreopt(false),
    savePending(false),
    readOnly(false),
    testingMode(false),
    test_nRunsStart(1),
    test_nRunsEnd(100),
    test_nStructs(600),
    cutoff(-1),
    m_schemaVersion(1),
    m_isDestroying(false)
  {
    // Connections
    connect(this, SIGNAL(sessionStarted()),
            m_queueThread, SLOT(start()),
            Qt::DirectConnection);
    connect(this, SIGNAL(startingSession()),
            this, SLOT(setIsStartingTrue()),
            Qt::DirectConnection);
    connect(this, SIGNAL(sessionStarted()),
            this, SLOT(setIsStartingFalse()),
            Qt::DirectConnection);
    connect(this, SIGNAL(readOnlySessionStarted()),
            this, SLOT(setIsStartingFalse()),
            Qt::DirectConnection);
    connect(this, SIGNAL(needBoolean(const QString&, bool*)),
            this, SLOT(promptForBoolean(const QString&, bool*)),
            Qt::BlockingQueuedConnection); // Wait until slot returns
    connect(this, SIGNAL(needPassword(const QString&, QString*, bool*)),
            this, SLOT(promptForPassword(const QString&, QString*, bool*)),
            Qt::BlockingQueuedConnection); // Wait until slot returns
    connect(this, SIGNAL(sig_setClipboard(const QString&)),
            this, SLOT(setClipboard_(const QString&)),
            Qt::QueuedConnection);

    INIT_RANDOM_GENERATOR();
  }

  OptBase::~OptBase()
  {
    m_isDestroying = true;

    delete m_queue;
    m_queue = 0;

    if (m_queueThread && m_queueThread->isRunning()) {
      m_queueThread->wait();
    }
    delete m_queueThread;
    m_queueThread = 0;

    delete m_optimizer;
    m_optimizer = 0;

    delete m_queueInterface;
    m_queueInterface = 0;

    delete m_tracker;
    m_tracker = 0;
  }

  void OptBase::reset() {
    m_tracker->lockForWrite();
    m_tracker->deleteAllStructures();
    m_tracker->reset();
    m_tracker->unlock();
    m_queue->reset();
  }

#ifdef ENABLE_SSH
  bool OptBase::createSSHConnections()
  {
#ifdef USE_CLI_SSH
    return this->createSSHConnections_cli();
#else // USE_CLI_SSH
    return this->createSSHConnections_libssh();
#endif // USE_CLI_SSH
  }
#endif // ENABLE_SSH

  void OptBase::printBackTrace() {
    backTraceMutex->lock();
    QStringList l = getBackTrace();
    backTraceMutex->unlock();
    for (int i = 0; i < l.size();i++)
      qDebug() << l.at(i);
  }

  QList<double> OptBase::getProbabilityList(const QList<Structure*> &structures) {
    // IMPORTANT: structures must contain one more structure than
    // needed -- the last structure in the list will be removed from
    // the probability list!
    if (structures.size() <= 2) {
      return QList<double>();
    }

    QList<double> probs;
    Structure *s=0, *first=0, *last=0;
    first = structures.first();
    last = structures.last();
    first->lock()->lockForRead();
    last->lock()->lockForRead();
    double lowest = first->getEnthalpy();
    double highest = last->getEnthalpy();;
    double spread = highest - lowest;
    last->lock()->unlock();
    first->lock()->unlock();
    // If all structures are at the same enthalpy, lets save some time...
    if (spread <= 1e-5) {
      double dprob = 1.0/static_cast<double>(structures.size()-1);
      double prob = 0;
      for (int i = 0; i < structures.size()-1; i++) {
        probs.append(prob);
        prob += dprob;
      }
      return probs;
    }
    // Generate a list of floats from 0->1 proportional to the enthalpies;
    // E.g. if enthalpies are:
    // -5   -2   -1   3   5
    // We'll have:
    // 0   0.3  0.4  0.8  1
    for (int i = 0; i < structures.size(); i++) {
      s = structures.at(i);
      s->lock()->lockForRead();
      probs.append( ( s->getEnthalpy() - lowest ) / spread);
      s->lock()->unlock();
    }
    // Subtract each value from one, and find the sum of the resulting list
    // We'll end up with:
    // 1  0.7  0.6  0.2  0   --   sum = 2.5
    double sum = 0;
    for (int i = 0; i < probs.size(); i++){
      probs[i] = 1.0 - probs.at(i);
      sum += probs.at(i);
    }
    // Normalize with the sum so that the list adds to 1
    // 0.4  0.28  0.24  0.08  0
    for (int i = 0; i < probs.size(); i++){
      probs[i] /= sum;
    }
    // Then replace each entry with a cumulative total:
    // 0.4 0.68 0.92 1 1
    sum = 0;
    for (int i = 0; i < probs.size(); i++){
      sum += probs.at(i);
      probs[i] = sum;
    }
    // Pop off the last entry (remember the n_popSize + 1 earlier?)
    // 0.4 0.68 0.92 1
    probs.removeLast();
    // And we have a enthalpy weighted probability list! To use:
    //
    //   double r = rand.NextFloat();
    //   uint ind;
    //   for (ind = 0; ind < probs.size(); ind++)
    //     if (r < probs.at(ind)) break;
    //
    // ind will hold the chosen index.
    return probs;
  }

  bool OptBase::save(const QString &stateFilename, bool notify)
  {
    if (isStarting ||
        readOnly) {
      savePending = false;
      return false;
    }
    QMutexLocker locker (stateFileMutex);
    QString filename;
    if (stateFilename.isEmpty()) {
      filename = filePath + "/" + m_idString.toLower() + ".state";
    }
    else {
      filename = stateFilename;
    }
    QString oldfilename = filename + ".old";

    if (notify) {
      if (!m_dialog->startProgressUpdate(tr("Saving: Writing %1...")
                                         .arg(filename),
                                         0, 0)) {
        // The progress bar is already in use -- disable notifications
        notify = false;
      }
    }

    // Copy .state -> .state.old
    if (QFile::exists(filename) ) {
      if (QFile::exists(oldfilename)) {
        QFile::remove(oldfilename);
      }
      QFile::copy(filename, oldfilename);
    }

    SETTINGS(filename);
    const int VERSION = m_schemaVersion;
    settings->beginGroup(m_idString.toLower());
    settings->setValue("version",          VERSION);
    settings->setValue("saveSuccessful", false);
    settings->endGroup();

    // Write/update .state
    m_dialog->writeSettings(filename);

    // Loop over structures and save them
    QReadLocker trackerLocker (m_tracker->rwLock());
    QList<Structure*> *structures = m_tracker->list();

    QString structureStateFileName;

    Structure* structure;
    for (int i = 0; i < structures->size(); i++) {
      structure = structures->at(i);
      structure->lock()->lockForRead();
      // Set index here -- this is the only time these are written, so
      // this is "ok" under a read lock because of the savePending logic
      structure->setIndex(i);
      structureStateFileName = structure->fileName() + "/structure.state";
      if (notify) {
        m_dialog->updateProgressLabel(tr("Saving: Writing %1...")
                                      .arg(structureStateFileName));
      }
      structure->writeSettings(structureStateFileName);
      structure->lock()->unlock();
    }

    /////////////////////////
    // Print results files //
    /////////////////////////

    QFile file (filePath + "/results.txt");
    QFile oldfile (filePath + "/results_old.txt");
    if (notify) {
      m_dialog->updateProgressLabel(tr("Saving: Writing %1...")
                                    .arg(file.fileName()));
    }
    if (oldfile.open(QIODevice::ReadOnly))
      oldfile.remove();
    if (file.open(QIODevice::ReadOnly))
      file.copy(oldfile.fileName());
    file.close();
    if (!file.open(QIODevice::WriteOnly)) {
      error("OptBase::save(): Error opening file "+file.fileName()+" for writing...");
      savePending = false;
      return false;
    }
    QTextStream out (&file);

    QList<Structure*> sortedStructures;

    for (int i = 0; i < structures->size(); i++)
      sortedStructures.append(structures->at(i));
    if (sortedStructures.size() != 0) {
      Structure::sortAndRankByEnthalpy(&sortedStructures);
      out << sortedStructures.first()->getResultsHeader() << endl;
    }

    for (int i = 0; i < sortedStructures.size(); i++) {
      structure = sortedStructures.at(i);
      if (!structure) continue; // In case there was a problem copying.
      structure->lock()->lockForRead();
      out << structure->getResultsEntry() << endl;
      structure->lock()->unlock();
      if (notify) {
        m_dialog->stopProgressUpdate();
      }
    }

    // Allow derived classes to do their own saving
    this->postSave(filename);

    // Mark operation successful
    settings->setValue(m_idString.toLower() + "/saveSuccessful", true);
    DESTROY_SETTINGS(filename);

    savePending = false;
    return true;
  }

  QString OptBase::interpretTemplate(const QString & str, Structure* structure)
  {
    QStringList list = str.split("%");
    QString line;
    QString origLine;
    for (int line_ind = 0; line_ind < list.size(); line_ind++) {
      origLine = line = list.at(line_ind);
      interpretKeyword_base(line, structure);
      // Add other interpret keyword sections here if needed when subclassing
      if (line != origLine) { // Line was a keyword
        list.replace(line_ind, line);
      }
    }
    // Rejoin string
    QString ret = list.join("");
    ret += "\n";
    return ret;
  }

  void OptBase::interpretKeyword_base(QString &line, Structure* structure)
  {
    QString rep = "";
    // User data
    if (line == "user1")                rep += optimizer()->getUser1();
    else if (line == "user2")           rep += optimizer()->getUser2();
    else if (line == "user3")           rep += optimizer()->getUser3();
    else if (line == "user4")           rep += optimizer()->getUser4();
    else if (line == "description")     rep += description;
    else if (line == "percent")         rep += "%";

    // Structure specific data
    if (line == "coords") {
      QList<Avogadro::Atom*> atoms = structure->atoms();
      QList<Avogadro::Atom*>::const_iterator it;
      int optIndex = -1;
      QHash<int, int> *lut = structure->getOptimizerLookupTable();
      lut->clear();
      const Eigen::Vector3d *vec;
      for (it  = atoms.begin();
           it != atoms.end();
           it++) {
        rep += QString(OpenBabel::etab.GetSymbol((*it)->atomicNumber()))+ " ";
        vec = (*it)->pos();
        rep += QString::number(vec->x()) + " ";
        rep += QString::number(vec->y()) + " ";
        rep += QString::number(vec->z()) + "\n";
        lut->insert(++optIndex, (*it)->index());
      }
    }
    else if (line == "coordsInternalFlags") {
      QList<Avogadro::Atom*> atoms = structure->atoms();
      QList<Avogadro::Atom*>::const_iterator it;
      const Eigen::Vector3d *vec;
      int optIndex = -1;
      QHash<int, int> *lut = structure->getOptimizerLookupTable();
      lut->clear();
      for (it  = atoms.begin();
           it != atoms.end();
           it++) {
        rep += QString(OpenBabel::etab.GetSymbol((*it)->atomicNumber()))+ " ";
        vec = (*it)->pos();
        rep += QString::number(vec->x()) + " 1 ";
        rep += QString::number(vec->y()) + " 1 ";
        rep += QString::number(vec->z()) + " 1\n";
        lut->insert(++optIndex, (*it)->index());
      }
    }
    else if (line == "coordsSuffixFlags") {
      QList<Avogadro::Atom*> atoms = structure->atoms();
      QList<Avogadro::Atom*>::const_iterator it;
      const Eigen::Vector3d *vec;
      int optIndex = -1;
      QHash<int, int> *lut = structure->getOptimizerLookupTable();
      lut->clear();
      for (it  = atoms.begin();
           it != atoms.end();
           it++) {
        rep += QString(OpenBabel::etab.GetSymbol((*it)->atomicNumber()))+ " ";
        vec = (*it)->pos();
        rep += QString::number(vec->x()) + " ";
        rep += QString::number(vec->y()) + " ";
        rep += QString::number(vec->z()) + " 1 1 1\n";
        lut->insert(++optIndex, (*it)->index());
      }
    }
    else if (line == "coordsId") {
      QList<Avogadro::Atom*> atoms = structure->atoms();
      QList<Avogadro::Atom*>::const_iterator it;
      const Eigen::Vector3d *vec;
      int optIndex = -1;
      QHash<int, int> *lut = structure->getOptimizerLookupTable();
      lut->clear();
      for (it  = atoms.begin();
           it != atoms.end();
           it++) {
        rep += QString(OpenBabel::etab.GetSymbol((*it)->atomicNumber()))+ " ";
        rep += QString::number((*it)->atomicNumber()) + " ";
        vec = (*it)->pos();
        rep += QString::number(vec->x()) + " ";
        rep += QString::number(vec->y()) + " ";
        rep += QString::number(vec->z()) + "\n";
        lut->insert(++optIndex, (*it)->index());
      }
    }
    else if (line == "numAtoms")	rep += QString::number(structure->numAtoms());
    else if (line == "numSpecies")	rep += QString::number(structure->getSymbols().size());
    else if (line == "filename")	rep += structure->fileName();
    else if (line == "rempath")       	rep += structure->getRempath();
    else if (line == "gen")           	rep += QString::number(structure->getGeneration());
    else if (line == "id")            	rep += QString::number(structure->getIDNumber());
    else if (line == "incar")         	rep += QString::number(structure->getCurrentOptStep());
    else if (line == "optStep")       	rep += QString::number(structure->getCurrentOptStep());

    if (!rep.isEmpty()) {
      // Remove any trailing newlines
      rep = rep.replace(QRegExp("\n$"), "");
      line = rep;
    }
  }

  QString OptBase::getTemplateKeywordHelp_base()
  {
    QString str;
    QTextStream out (&str);
    out
      << "The following keywords should be used instead of the indicated variable data:\n"
      << "\n"
      << "Misc:\n"
      << "%percent% -- Literal percent sign (needed for CASTEP!)\n"
      << "\n"
      << "User data:\n"
      << "%userX% -- User specified value, where X = 1, 2, 3, or 4\n"
      << "%description% -- Optimization description\n"
      << "\n"
      << "Atomic coordinate formats for isolated structures:\n"
      << "%coords% -- cartesian coordinates\n\t[symbol] [x] [y] [z]\n"
      << "%coordsInternalFlags% -- cartesian coordinates; flag after each coordinate\n\t[symbol] [x] 1 [y] 1 [z] 1\n"
      << "%coordsSuffixFlags% -- cartesian coordinates; flags after all coordinates\n\t[symbol] [x] [y] [z] 1 1 1\n"
      << "%coordsId% -- cartesian coordinates with atomic number\n\t[symbol] [atomic number] [x] [y] [z]\n"
      << "\n"
      << "Generic structure data:\n"
      << "%numAtoms% -- Number of atoms in unit cell\n"
      << "%numSpecies% -- Number of unique atomic species in unit cell\n"
      << "%filename% -- local output filename\n"
      << "%rempath% -- path to structure's remote directory\n"
      << "%gen% -- structure generation number (if relevant)\n"
      << "%id% -- structure id number\n"
      << "%optStep% -- current optimization step\n"
      ;
    return str;
  }

  void OptBase::setOptimizer(Optimizer *o)
  {
    m_optimizer = o;
    emit optimizerChanged(o);
  }

  void OptBase::setQueueInterface(QueueInterface *q)
  {
    m_queueInterface = q;
    emit queueInterfaceChanged(q);
  }

  void OptBase::promptForPassword(const QString &message,
                                  QString *newPassword,
                                  bool *ok)
  {
    (*newPassword) = QInputDialog::getText(dialog(), "Need password:", message,
                                           QLineEdit::Password, QString(), ok);
  };

  void OptBase::promptForBoolean(const QString &message,
                                 bool *ok)
  {
    if (QMessageBox::question(dialog(), m_idString, message,
                              QMessageBox::Yes | QMessageBox::No)
        == QMessageBox::Yes) {
      *ok = true;
    } else {
      *ok = false;
    }
  }

  void OptBase::setClipboard(const QString &text) const
  {
    emit sig_setClipboard(text);
  }

  // No need to document this
  /// @cond
  void OptBase::setClipboard_(const QString &text) const
  {
    // Set to system clipboard
    QApplication::clipboard()->setText(text, QClipboard::Clipboard);
    // For middle-click on X11
    if (QApplication::clipboard()->supportsSelection()) {
      QApplication::clipboard()->setText(text, QClipboard::Selection);
    }
  }
  /// @endcond

#ifdef ENABLE_SSH
#ifndef USE_CLI_SSH

  bool OptBase::createSSHConnections_libssh()
  {
    delete m_ssh;
    SSHManagerLibSSH *libsshManager = new SSHManagerLibSSH(5, this);
    m_ssh = libsshManager;
    QString pw = "";
    for (;;) {
      try {
        libsshManager->makeConnections(host, username, pw, port);
      }
      catch (SSHConnection::SSHConnectionException e) {
        QString err;
        switch (e) {
        case SSHConnection::SSH_CONNECTION_ERROR:
        case SSHConnection::SSH_UNKNOWN_ERROR:
        default:
          err = "There was a problem connection to the ssh server at "
              + username + "@" + host + ":" + QString::number(port) + ". "
              "Please check that all provided information is correct, and "
              "attempt to log in outside of Avogadro before trying again.";
          error(err);
          return false;
        case SSHConnection::SSH_UNKNOWN_HOST_ERROR: {
          // The host is not known, or has changed its key.
          // Ask user if this is ok.
          err = "The host "
            + host + ":" + QString::number(port)
            + " either has an unknown key, or has changed it's key:\n"
            + libsshManager->getServerKeyHash() + "\n"
            + "Would you like to trust the specified host?";
          bool ok;
          // This is a BlockingQueuedConnection, which blocks until
          // the slot returns.
          emit needBoolean(err, &ok);
          if (!ok) { // user cancels
            return false;
          }
          libsshManager->validateServerKey();
          continue;
        } // end case
        case SSHConnection::SSH_BAD_PASSWORD_ERROR: {
          // Chances are that the pubkey auth was attempted but failed,
          // so just prompt user for password.
          err = "Please enter a password for "
            + username + "@" + host + ":" + QString::number(port)
            + ":";
          bool ok;
          QString newPassword;
          // This is a BlockingQueuedConnection, which blocks until
          // the slot returns.
          emit needPassword(err, &newPassword, &ok);
          if (!ok) { // user cancels
            return false;
          }
          pw = newPassword;
          continue;
        } // end case
        } // end switch
      } // end catch
      break;
    } // end forever
    return true;
  }

#else // not USE_CLI_SSH

  bool OptBase::createSSHConnections_cli()
  {
    // Since we rely on public key auth here, it's much easier to set up:
    SSHManagerCLI *cliSSHManager = new SSHManagerCLI(5, this);
    cliSSHManager->makeConnections(host, username, "", port);
    m_ssh = cliSSHManager;
    return true;
  }

#endif // not USE_CLI_SSH
#endif // ENABLE_SSH

  void OptBase::warning(const QString & s) {
    qWarning() << "Warning: " << s;
    emit warningStatement(s);
  }

  void OptBase::debug(const QString & s) {
    qDebug() << "Debug: " << s;
    emit debugStatement(s);
  }

  void OptBase::error(const QString & s) {
    qWarning() << "Error: " << s;
    emit errorStatement(s);
  }

} // end namespace GlobalSearch

