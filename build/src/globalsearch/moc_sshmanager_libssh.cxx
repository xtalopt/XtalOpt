/****************************************************************************
** Meta object code from reading C++ file 'sshmanager_libssh.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.6)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/globalsearch/sshmanager_libssh.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'sshmanager_libssh.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.6. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_GlobalSearch__SSHManagerLibSSH[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      31,   51,   52,   51, 0x0a,
      67,  100,   51,   51, 0x0a,
     104,   51,  123,   51, 0x0a,
     131,   51,  151,   51, 0x0a,
     156,  178,   51,   51, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_GlobalSearch__SSHManagerLibSSH[] = {
    "GlobalSearch::SSHManagerLibSSH\0"
    "getFreeConnection()\0\0SSHConnection*\0"
    "unlockConnection(SSHConnection*)\0ssh\0"
    "getServerKeyHash()\0QString\0"
    "validateServerKey()\0bool\0setServerKey(QString)\0"
    "hexa\0"
};

void GlobalSearch::SSHManagerLibSSH::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        SSHManagerLibSSH *_t = static_cast<SSHManagerLibSSH *>(_o);
        switch (_id) {
        case 0: { SSHConnection* _r = _t->getFreeConnection();
            if (_a[0]) *reinterpret_cast< SSHConnection**>(_a[0]) = _r; }  break;
        case 1: _t->unlockConnection((*reinterpret_cast< SSHConnection*(*)>(_a[1]))); break;
        case 2: { QString _r = _t->getServerKeyHash();
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = _r; }  break;
        case 3: { bool _r = _t->validateServerKey();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 4: _t->setServerKey((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData GlobalSearch::SSHManagerLibSSH::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject GlobalSearch::SSHManagerLibSSH::staticMetaObject = {
    { &SSHManager::staticMetaObject, qt_meta_stringdata_GlobalSearch__SSHManagerLibSSH,
      qt_meta_data_GlobalSearch__SSHManagerLibSSH, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &GlobalSearch::SSHManagerLibSSH::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *GlobalSearch::SSHManagerLibSSH::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *GlobalSearch::SSHManagerLibSSH::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_GlobalSearch__SSHManagerLibSSH))
        return static_cast<void*>(const_cast< SSHManagerLibSSH*>(this));
    return SSHManager::qt_metacast(_clname);
}

int GlobalSearch::SSHManagerLibSSH::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = SSHManager::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
