/****************************************************************************
** Meta object code from reading C++ file 'optbase.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.6)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/globalsearch/optbase.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'optbase.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.6. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_GlobalSearch__OptBase[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      36,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      12,       // signalCount

 // signals: signature, parameters, type, tag, flags
      22,   40,   40,   40, 0x05,
      41,   40,   40,   40, 0x05,
      58,   40,   40,   40, 0x05,
      83,   40,   40,   40, 0x05,
     122,   40,   40,   40, 0x05,
     151,  175,   40,   40, 0x05,
     177,  175,   40,   40, 0x05,
     203,  175,   40,   40, 0x05,
     227,  254,   40,   40, 0x05,
     265,  302,   40,   40, 0x05,
     325,   40,   40,   40, 0x05,
     351,  377,   40,   40, 0x05,

 // slots: signature, parameters, type, tag, flags
     382,   40,   40,   40, 0x0a,
     390,   40,  413,   40, 0x0a,
     418,   40,   40,   40, 0x0a,
     432,   40,   40,   40, 0x0a,
     455,  175,   40,   40, 0x0a,
     470,  175,   40,   40, 0x0a,
     487,  175,   40,   40, 0x0a,
     502,   40,   40,   40, 0x0a,
     523,   40,   40,   40, 0x0a,
     552,   40,   40,   40, 0x0a,
     574,   40,   40,   40, 0x0a,
     594,   40,   40,   40, 0x0a,
     615,   40,   40,   40, 0x0a,
     633,   40,   40,   40, 0x0a,
     652,   40,   40,   40, 0x0a,
     669,  704,   40,   40, 0x0a,
     706,  731,   40,   40, 0x0a,
     733,  254,   40,   40, 0x0a,
     765,  791,   40,   40, 0x2a,
     799,  302,   40,   40, 0x0a,
     841,  877,   40,   40, 0x2a,
     897,  377,   40,   40, 0x0a,
     919,  377,   40,   40, 0x09,
     942,   40,  413,   40, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_GlobalSearch__OptBase[] = {
    "GlobalSearch::OptBase\0startingSession()\0"
    "\0sessionStarted()\0readOnlySessionStarted()\0"
    "queueInterfaceChanged(QueueInterface*)\0"
    "optimizerChanged(Optimizer*)\0"
    "debugStatement(QString)\0s\0"
    "warningStatement(QString)\0"
    "errorStatement(QString)\0"
    "needBoolean(QString,bool*)\0message,ok\0"
    "needPassword(QString,QString*,bool*)\0"
    "message,newPassword,ok\0refreshAllStructureInfo()\0"
    "sig_setClipboard(QString)\0text\0reset()\0"
    "createSSHConnections()\0bool\0startSearch()\0"
    "generateNewStructure()\0debug(QString)\0"
    "warning(QString)\0error(QString)\0"
    "emitSessionStarted()\0emitReadOnlySessionStarted()\0"
    "emitStartingSession()\0setIsStartingTrue()\0"
    "setIsStartingFalse()\0setReadOnlyTrue()\0"
    "setReadOnlyFalse()\0printBackTrace()\0"
    "setQueueInterface(QueueInterface*)\0q\0"
    "setOptimizer(Optimizer*)\0o\0"
    "promptForBoolean(QString,bool*)\0"
    "promptForBoolean(QString)\0message\0"
    "promptForPassword(QString,QString*,bool*)\0"
    "promptForPassword(QString,QString*)\0"
    "message,newPassword\0setClipboard(QString)\0"
    "setClipboard_(QString)\0"
    "createSSHConnections_libssh()\0"
};

void GlobalSearch::OptBase::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        OptBase *_t = static_cast<OptBase *>(_o);
        switch (_id) {
        case 0: _t->startingSession(); break;
        case 1: _t->sessionStarted(); break;
        case 2: _t->readOnlySessionStarted(); break;
        case 3: _t->queueInterfaceChanged((*reinterpret_cast< QueueInterface*(*)>(_a[1]))); break;
        case 4: _t->optimizerChanged((*reinterpret_cast< Optimizer*(*)>(_a[1]))); break;
        case 5: _t->debugStatement((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 6: _t->warningStatement((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 7: _t->errorStatement((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 8: _t->needBoolean((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< bool*(*)>(_a[2]))); break;
        case 9: _t->needPassword((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< QString*(*)>(_a[2])),(*reinterpret_cast< bool*(*)>(_a[3]))); break;
        case 10: _t->refreshAllStructureInfo(); break;
        case 11: _t->sig_setClipboard((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 12: _t->reset(); break;
        case 13: { bool _r = _t->createSSHConnections();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 14: _t->startSearch(); break;
        case 15: _t->generateNewStructure(); break;
        case 16: _t->debug((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 17: _t->warning((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 18: _t->error((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 19: _t->emitSessionStarted(); break;
        case 20: _t->emitReadOnlySessionStarted(); break;
        case 21: _t->emitStartingSession(); break;
        case 22: _t->setIsStartingTrue(); break;
        case 23: _t->setIsStartingFalse(); break;
        case 24: _t->setReadOnlyTrue(); break;
        case 25: _t->setReadOnlyFalse(); break;
        case 26: _t->printBackTrace(); break;
        case 27: _t->setQueueInterface((*reinterpret_cast< QueueInterface*(*)>(_a[1]))); break;
        case 28: _t->setOptimizer((*reinterpret_cast< Optimizer*(*)>(_a[1]))); break;
        case 29: _t->promptForBoolean((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< bool*(*)>(_a[2]))); break;
        case 30: _t->promptForBoolean((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 31: _t->promptForPassword((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< QString*(*)>(_a[2])),(*reinterpret_cast< bool*(*)>(_a[3]))); break;
        case 32: _t->promptForPassword((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< QString*(*)>(_a[2]))); break;
        case 33: _t->setClipboard((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 34: _t->setClipboard_((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 35: { bool _r = _t->createSSHConnections_libssh();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        default: ;
        }
    }
}

const QMetaObjectExtraData GlobalSearch::OptBase::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject GlobalSearch::OptBase::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_GlobalSearch__OptBase,
      qt_meta_data_GlobalSearch__OptBase, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &GlobalSearch::OptBase::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *GlobalSearch::OptBase::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *GlobalSearch::OptBase::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_GlobalSearch__OptBase))
        return static_cast<void*>(const_cast< OptBase*>(this));
    return QObject::qt_metacast(_clname);
}

int GlobalSearch::OptBase::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 36)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 36;
    }
    return _id;
}

// SIGNAL 0
void GlobalSearch::OptBase::startingSession()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}

// SIGNAL 1
void GlobalSearch::OptBase::sessionStarted()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}

// SIGNAL 2
void GlobalSearch::OptBase::readOnlySessionStarted()
{
    QMetaObject::activate(this, &staticMetaObject, 2, 0);
}

// SIGNAL 3
void GlobalSearch::OptBase::queueInterfaceChanged(QueueInterface * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void GlobalSearch::OptBase::optimizerChanged(Optimizer * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void GlobalSearch::OptBase::debugStatement(const QString & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void GlobalSearch::OptBase::warningStatement(const QString & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void GlobalSearch::OptBase::errorStatement(const QString & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void GlobalSearch::OptBase::needBoolean(const QString & _t1, bool * _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void GlobalSearch::OptBase::needPassword(const QString & _t1, QString * _t2, bool * _t3)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}

// SIGNAL 10
void GlobalSearch::OptBase::refreshAllStructureInfo()
{
    QMetaObject::activate(this, &staticMetaObject, 10, 0);
}

// SIGNAL 11
void GlobalSearch::OptBase::sig_setClipboard(const QString & _t1)const
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(const_cast< GlobalSearch::OptBase *>(this), &staticMetaObject, 11, _a);
}
QT_END_MOC_NAMESPACE
