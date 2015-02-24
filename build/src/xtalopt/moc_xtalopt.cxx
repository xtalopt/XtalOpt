/****************************************************************************
** Meta object code from reading C++ file 'xtalopt.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.6)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/xtalopt/xtalopt.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'xtalopt.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.6. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_XtalOpt__XtalOpt[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      17,   31,   31,   31, 0x0a,
      32,   31,   31,   31, 0x0a,
      55,   31,   73,   31, 0x0a,
      79,  120,   31,   31, 0x0a,
     144,  120,   31,   31, 0x0a,
     182,   31,   31,   31, 0x0a,
     201,   31,   31,   31, 0x0a,
     219,   31,   31,   31, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_XtalOpt__XtalOpt[] = {
    "XtalOpt::XtalOpt\0startSearch()\0\0"
    "generateNewStructure()\0generateNewXtal()\0"
    "Xtal*\0initializeAndAddXtal(Xtal*,uint,QString)\0"
    "xtal,generation,parents\0"
    "initializeSubXtal(Xtal*,uint,QString)\0"
    "resetSpacegroups()\0resetDuplicates()\0"
    "checkForDuplicates()\0"
};

void XtalOpt::XtalOpt::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        XtalOpt *_t = static_cast<XtalOpt *>(_o);
        switch (_id) {
        case 0: _t->startSearch(); break;
        case 1: _t->generateNewStructure(); break;
        case 2: { Xtal* _r = _t->generateNewXtal();
            if (_a[0]) *reinterpret_cast< Xtal**>(_a[0]) = _r; }  break;
        case 3: _t->initializeAndAddXtal((*reinterpret_cast< Xtal*(*)>(_a[1])),(*reinterpret_cast< uint(*)>(_a[2])),(*reinterpret_cast< const QString(*)>(_a[3]))); break;
        case 4: _t->initializeSubXtal((*reinterpret_cast< Xtal*(*)>(_a[1])),(*reinterpret_cast< uint(*)>(_a[2])),(*reinterpret_cast< const QString(*)>(_a[3]))); break;
        case 5: _t->resetSpacegroups(); break;
        case 6: _t->resetDuplicates(); break;
        case 7: _t->checkForDuplicates(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData XtalOpt::XtalOpt::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject XtalOpt::XtalOpt::staticMetaObject = {
    { &GlobalSearch::OptBase::staticMetaObject, qt_meta_stringdata_XtalOpt__XtalOpt,
      qt_meta_data_XtalOpt__XtalOpt, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &XtalOpt::XtalOpt::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *XtalOpt::XtalOpt::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *XtalOpt::XtalOpt::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_XtalOpt__XtalOpt))
        return static_cast<void*>(const_cast< XtalOpt*>(this));
    typedef GlobalSearch::OptBase QMocSuperClass;
    return QMocSuperClass::qt_metacast(_clname);
}

int XtalOpt::XtalOpt::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    typedef GlobalSearch::OptBase QMocSuperClass;
    _id = QMocSuperClass::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
