/****************************************************************************
** Meta object code from reading C++ file 'queuemanager.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.6)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/globalsearch/queuemanager.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'queuemanager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.6. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_GlobalSearch__QueueManager[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      25,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       8,       // signalCount

 // signals: signature, parameters, type, tag, flags
      27,   45,   45,   45, 0x05,
      46,   89,   45,   45, 0x05,
      91,   89,   45,   45, 0x05,
     136,   89,   45,   45, 0x05,
     178,   89,   45,   45, 0x05,
     221,   89,   45,   45, 0x05,
     265,   45,   45,   45, 0x05,
     284,  315,   45,   45, 0x05,

 // slots: signature, parameters, type, tag, flags
     341,   45,   45,   45, 0x0a,
     349,   89,   45,   45, 0x0a,
     375,   89,   45,   45, 0x0a,
     411,  442,   45,   45, 0x0a,
     451,   45,   45,   45, 0x2a,
     479,   45,  505,   45, 0x0a,
     523,   45,  505,   45, 0x0a,
     551,   45,  505,   45, 0x0a,
     579,   45,  505,   45, 0x0a,
     598,   45,  505,   45, 0x0a,
     614,   89,   45,   45, 0x0a,
     642,   45,   45,   45, 0x2a,
     660,   45,   45,   45, 0x09,
     672,  732,   45,   45, 0x09,
     742,   89,   45,   45, 0x09,
     798,   45,   45,   45, 0x09,
     815,   45,   45,   45, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_GlobalSearch__QueueManager[] = {
    "GlobalSearch::QueueManager\0movedToQMThread()\0"
    "\0structureStarted(GlobalSearch::Structure*)\0"
    "s\0structureSubmitted(GlobalSearch::Structure*)\0"
    "structureKilled(GlobalSearch::Structure*)\0"
    "structureUpdated(GlobalSearch::Structure*)\0"
    "structureFinished(GlobalSearch::Structure*)\0"
    "needNewStructure()\0newStatusOverview(int,int,int)\0"
    "optimized,running,failing\0reset()\0"
    "killStructure(Structure*)\0"
    "appendToJobStartTracker(Structure*)\0"
    "addManualStructureRequest(int)\0requests\0"
    "addManualStructureRequest()\0"
    "getAllRunningStructures()\0QList<Structure*>\0"
    "getAllOptimizedStructures()\0"
    "getAllDuplicateStructures()\0"
    "getAllStructures()\0lockForNaming()\0"
    "unlockForNaming(Structure*)\0"
    "unlockForNaming()\0checkLoop()\0"
    "addStructureToSubmissionQueue(GlobalSearch::Structure*,int)\0"
    "s,optStep\0"
    "addStructureToSubmissionQueue(GlobalSearch::Structure*)\0"
    "moveToQMThread()\0setupConnections()\0"
};

void GlobalSearch::QueueManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        QueueManager *_t = static_cast<QueueManager *>(_o);
        switch (_id) {
        case 0: _t->movedToQMThread(); break;
        case 1: _t->structureStarted((*reinterpret_cast< GlobalSearch::Structure*(*)>(_a[1]))); break;
        case 2: _t->structureSubmitted((*reinterpret_cast< GlobalSearch::Structure*(*)>(_a[1]))); break;
        case 3: _t->structureKilled((*reinterpret_cast< GlobalSearch::Structure*(*)>(_a[1]))); break;
        case 4: _t->structureUpdated((*reinterpret_cast< GlobalSearch::Structure*(*)>(_a[1]))); break;
        case 5: _t->structureFinished((*reinterpret_cast< GlobalSearch::Structure*(*)>(_a[1]))); break;
        case 6: _t->needNewStructure(); break;
        case 7: _t->newStatusOverview((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 8: _t->reset(); break;
        case 9: _t->killStructure((*reinterpret_cast< Structure*(*)>(_a[1]))); break;
        case 10: _t->appendToJobStartTracker((*reinterpret_cast< Structure*(*)>(_a[1]))); break;
        case 11: _t->addManualStructureRequest((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 12: _t->addManualStructureRequest(); break;
        case 13: { QList<Structure*> _r = _t->getAllRunningStructures();
            if (_a[0]) *reinterpret_cast< QList<Structure*>*>(_a[0]) = _r; }  break;
        case 14: { QList<Structure*> _r = _t->getAllOptimizedStructures();
            if (_a[0]) *reinterpret_cast< QList<Structure*>*>(_a[0]) = _r; }  break;
        case 15: { QList<Structure*> _r = _t->getAllDuplicateStructures();
            if (_a[0]) *reinterpret_cast< QList<Structure*>*>(_a[0]) = _r; }  break;
        case 16: { QList<Structure*> _r = _t->getAllStructures();
            if (_a[0]) *reinterpret_cast< QList<Structure*>*>(_a[0]) = _r; }  break;
        case 17: { QList<Structure*> _r = _t->lockForNaming();
            if (_a[0]) *reinterpret_cast< QList<Structure*>*>(_a[0]) = _r; }  break;
        case 18: _t->unlockForNaming((*reinterpret_cast< Structure*(*)>(_a[1]))); break;
        case 19: _t->unlockForNaming(); break;
        case 20: _t->checkLoop(); break;
        case 21: _t->addStructureToSubmissionQueue((*reinterpret_cast< GlobalSearch::Structure*(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 22: _t->addStructureToSubmissionQueue((*reinterpret_cast< GlobalSearch::Structure*(*)>(_a[1]))); break;
        case 23: _t->moveToQMThread(); break;
        case 24: _t->setupConnections(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData GlobalSearch::QueueManager::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject GlobalSearch::QueueManager::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_GlobalSearch__QueueManager,
      qt_meta_data_GlobalSearch__QueueManager, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &GlobalSearch::QueueManager::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *GlobalSearch::QueueManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *GlobalSearch::QueueManager::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_GlobalSearch__QueueManager))
        return static_cast<void*>(const_cast< QueueManager*>(this));
    return QObject::qt_metacast(_clname);
}

int GlobalSearch::QueueManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 25)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 25;
    }
    return _id;
}

// SIGNAL 0
void GlobalSearch::QueueManager::movedToQMThread()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}

// SIGNAL 1
void GlobalSearch::QueueManager::structureStarted(GlobalSearch::Structure * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void GlobalSearch::QueueManager::structureSubmitted(GlobalSearch::Structure * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void GlobalSearch::QueueManager::structureKilled(GlobalSearch::Structure * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void GlobalSearch::QueueManager::structureUpdated(GlobalSearch::Structure * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void GlobalSearch::QueueManager::structureFinished(GlobalSearch::Structure * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void GlobalSearch::QueueManager::needNewStructure()
{
    QMetaObject::activate(this, &staticMetaObject, 6, 0);
}

// SIGNAL 7
void GlobalSearch::QueueManager::newStatusOverview(int _t1, int _t2, int _t3)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}
QT_END_MOC_NAMESPACE
