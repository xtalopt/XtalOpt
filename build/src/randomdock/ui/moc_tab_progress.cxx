/****************************************************************************
** Meta object code from reading C++ file 'tab_progress.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.6)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/randomdock/ui/tab_progress.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'tab_progress.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.6. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_RandomDock__TabProgress[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      27,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: signature, parameters, type, tag, flags
      24,   39,   39,   39, 0x05,
      40,   70,   39,   39, 0x05,
      89,   39,   39,   39, 0x05,
     102,  143,   39,   39, 0x05,

 // slots: signature, parameters, type, tag, flags
     149,  171,   39,   39, 0x0a,
     180,   39,   39,   39, 0x2a,
     195,  171,   39,   39, 0x0a,
     218,   39,   39,   39, 0x2a,
     234,   39,   39,   39, 0x0a,
     250,   39,   39,   39, 0x0a,
     264,   39,   39,   39, 0x0a,
     304,   39,   39,   39, 0x0a,
     317,   39,   39,   39, 0x0a,
     333,   39,   39,   39, 0x0a,
     355,  143,   39,   39, 0x0a,
     393,  437,   39,   39, 0x0a,
     441,  482,   39,   39, 0x0a,
     492,   39,   39,   39, 0x0a,
     505,   39,   39,   39, 0x0a,
     517,   39,   39,   39, 0x0a,
     545,   39,   39,   39, 0x0a,
     566,   39,   39,   39, 0x0a,
     586,   39,   39,   39, 0x0a,
     608,   39,   39,   39, 0x0a,
     636,   39,   39,   39, 0x0a,
     665,   39,   39,   39, 0x0a,
     685,   39,   39,   39, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_RandomDock__TabProgress[] = {
    "RandomDock::TabProgress\0deleteJob(int)\0"
    "\0updateStatus(int,int,int,int)\0"
    "opt,run,queue,fail\0infoUpdate()\0"
    "updateTableEntry(int,RD_Prog_TableEntry)\0"
    "row,e\0readSettings(QString)\0filename\0"
    "readSettings()\0writeSettings(QString)\0"
    "writeSettings()\0disconnectGUI()\0"
    "addNewEntry()\0newInfoUpdate(GlobalSearch::Structure*)\0"
    "updateInfo()\0updateAllInfo()\0"
    "updateProgressTable()\0"
    "setTableEntry(int,RD_Prog_TableEntry)\0"
    "selectMoleculeFromProgress(int,int,int,int)\0"
    ",,,\0highlightScene(GlobalSearch::Structure*)\0"
    "structure\0startTimer()\0stopTimer()\0"
    "progressContextMenu(QPoint)\0"
    "restartJobProgress()\0killSceneProgress()\0"
    "unkillSceneProgress()\0resetFailureCountProgress()\0"
    "randomizeStructureProgress()\0"
    "enableRowTracking()\0disableRowTracking()\0"
};

void RandomDock::TabProgress::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        TabProgress *_t = static_cast<TabProgress *>(_o);
        switch (_id) {
        case 0: _t->deleteJob((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->updateStatus((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4]))); break;
        case 2: _t->infoUpdate(); break;
        case 3: _t->updateTableEntry((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const RD_Prog_TableEntry(*)>(_a[2]))); break;
        case 4: _t->readSettings((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 5: _t->readSettings(); break;
        case 6: _t->writeSettings((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 7: _t->writeSettings(); break;
        case 8: _t->disconnectGUI(); break;
        case 9: _t->addNewEntry(); break;
        case 10: _t->newInfoUpdate((*reinterpret_cast< GlobalSearch::Structure*(*)>(_a[1]))); break;
        case 11: _t->updateInfo(); break;
        case 12: _t->updateAllInfo(); break;
        case 13: _t->updateProgressTable(); break;
        case 14: _t->setTableEntry((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const RD_Prog_TableEntry(*)>(_a[2]))); break;
        case 15: _t->selectMoleculeFromProgress((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4]))); break;
        case 16: _t->highlightScene((*reinterpret_cast< GlobalSearch::Structure*(*)>(_a[1]))); break;
        case 17: _t->startTimer(); break;
        case 18: _t->stopTimer(); break;
        case 19: _t->progressContextMenu((*reinterpret_cast< QPoint(*)>(_a[1]))); break;
        case 20: _t->restartJobProgress(); break;
        case 21: _t->killSceneProgress(); break;
        case 22: _t->unkillSceneProgress(); break;
        case 23: _t->resetFailureCountProgress(); break;
        case 24: _t->randomizeStructureProgress(); break;
        case 25: _t->enableRowTracking(); break;
        case 26: _t->disableRowTracking(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData RandomDock::TabProgress::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject RandomDock::TabProgress::staticMetaObject = {
    { &GlobalSearch::AbstractTab::staticMetaObject, qt_meta_stringdata_RandomDock__TabProgress,
      qt_meta_data_RandomDock__TabProgress, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &RandomDock::TabProgress::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *RandomDock::TabProgress::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *RandomDock::TabProgress::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_RandomDock__TabProgress))
        return static_cast<void*>(const_cast< TabProgress*>(this));
    typedef GlobalSearch::AbstractTab QMocSuperClass;
    return QMocSuperClass::qt_metacast(_clname);
}

int RandomDock::TabProgress::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    typedef GlobalSearch::AbstractTab QMocSuperClass;
    _id = QMocSuperClass::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 27)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 27;
    }
    return _id;
}

// SIGNAL 0
void RandomDock::TabProgress::deleteJob(int _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void RandomDock::TabProgress::updateStatus(int _t1, int _t2, int _t3, int _t4)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)), const_cast<void*>(reinterpret_cast<const void*>(&_t4)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void RandomDock::TabProgress::infoUpdate()
{
    QMetaObject::activate(this, &staticMetaObject, 2, 0);
}

// SIGNAL 3
void RandomDock::TabProgress::updateTableEntry(int _t1, const RD_Prog_TableEntry & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
QT_END_MOC_NAMESPACE
