/****************************************************************************
** Meta object code from reading C++ file 'sshmanager.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.6)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/globalsearch/sshmanager.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'sshmanager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.6. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_GlobalSearch__SSHManager[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      25,   45,   46,   45, 0x0a,
      61,   94,   45,   45, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_GlobalSearch__SSHManager[] = {
    "GlobalSearch::SSHManager\0getFreeConnection()\0"
    "\0SSHConnection*\0unlockConnection(SSHConnection*)\0"
    "ssh\0"
};

void GlobalSearch::SSHManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        SSHManager *_t = static_cast<SSHManager *>(_o);
        switch (_id) {
        case 0: { SSHConnection* _r = _t->getFreeConnection();
            if (_a[0]) *reinterpret_cast< SSHConnection**>(_a[0]) = _r; }  break;
        case 1: _t->unlockConnection((*reinterpret_cast< SSHConnection*(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData GlobalSearch::SSHManager::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject GlobalSearch::SSHManager::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_GlobalSearch__SSHManager,
      qt_meta_data_GlobalSearch__SSHManager, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &GlobalSearch::SSHManager::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *GlobalSearch::SSHManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *GlobalSearch::SSHManager::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_GlobalSearch__SSHManager))
        return static_cast<void*>(const_cast< SSHManager*>(this));
    return QObject::qt_metacast(_clname);
}

int GlobalSearch::SSHManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
