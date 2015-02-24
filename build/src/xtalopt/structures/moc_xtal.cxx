/****************************************************************************
** Meta object code from reading C++ file 'xtal.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.6)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/xtalopt/structures/xtal.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'xtal.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.6. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_XtalOpt__Xtal[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      11,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: signature, parameters, type, tag, flags
      14,   34,   34,   34, 0x05,

 // slots: signature, parameters, type, tag, flags
      35,   90,   34,   34, 0x0a,
     113,  147,   34,   34, 0x0a,
     149,  219,   34,   34, 0x0a,
     228,  246,   34,   34, 0x0a,
     253,  308,   34,   34, 0x0a,
     331,  346,  355,   34, 0x0a,
     360,   34,  355,   34, 0x2a,
     372,   34,   34,   34, 0x0a,
     390,  413,   34,   34, 0x0a,
     418,   34,   34,   34, 0x2a,

       0        // eod
};

static const char qt_meta_stringdata_XtalOpt__Xtal[] = {
    "XtalOpt::Xtal\0dimensionsChanged()\0\0"
    "setCellInfo(double,double,double,double,double,double)\0"
    "A,B,C,Alpha,Beta,Gamma\0"
    "setCellInfo(OpenBabel::matrix3x3)\0m\0"
    "setCellInfo(OpenBabel::vector3,OpenBabel::vector3,OpenBabel::vector3)\0"
    "v1,v2,v3\0setVolume(double)\0Volume\0"
    "rescaleCell(double,double,double,double,double,double)\0"
    "a,b,c,alpha,beta,gamma\0fixAngles(int)\0"
    "attempts\0bool\0fixAngles()\0wrapAtomsToCell()\0"
    "findSpaceGroup(double)\0prec\0"
    "findSpaceGroup()\0"
};

void XtalOpt::Xtal::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        Xtal *_t = static_cast<Xtal *>(_o);
        switch (_id) {
        case 0: _t->dimensionsChanged(); break;
        case 1: _t->setCellInfo((*reinterpret_cast< double(*)>(_a[1])),(*reinterpret_cast< double(*)>(_a[2])),(*reinterpret_cast< double(*)>(_a[3])),(*reinterpret_cast< double(*)>(_a[4])),(*reinterpret_cast< double(*)>(_a[5])),(*reinterpret_cast< double(*)>(_a[6]))); break;
        case 2: _t->setCellInfo((*reinterpret_cast< const OpenBabel::matrix3x3(*)>(_a[1]))); break;
        case 3: _t->setCellInfo((*reinterpret_cast< const OpenBabel::vector3(*)>(_a[1])),(*reinterpret_cast< const OpenBabel::vector3(*)>(_a[2])),(*reinterpret_cast< const OpenBabel::vector3(*)>(_a[3]))); break;
        case 4: _t->setVolume((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 5: _t->rescaleCell((*reinterpret_cast< double(*)>(_a[1])),(*reinterpret_cast< double(*)>(_a[2])),(*reinterpret_cast< double(*)>(_a[3])),(*reinterpret_cast< double(*)>(_a[4])),(*reinterpret_cast< double(*)>(_a[5])),(*reinterpret_cast< double(*)>(_a[6]))); break;
        case 6: { bool _r = _t->fixAngles((*reinterpret_cast< int(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 7: { bool _r = _t->fixAngles();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 8: _t->wrapAtomsToCell(); break;
        case 9: _t->findSpaceGroup((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 10: _t->findSpaceGroup(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData XtalOpt::Xtal::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject XtalOpt::Xtal::staticMetaObject = {
    { &GlobalSearch::Structure::staticMetaObject, qt_meta_stringdata_XtalOpt__Xtal,
      qt_meta_data_XtalOpt__Xtal, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &XtalOpt::Xtal::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *XtalOpt::Xtal::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *XtalOpt::Xtal::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_XtalOpt__Xtal))
        return static_cast<void*>(const_cast< Xtal*>(this));
    typedef GlobalSearch::Structure QMocSuperClass;
    return QMocSuperClass::qt_metacast(_clname);
}

int XtalOpt::Xtal::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    typedef GlobalSearch::Structure QMocSuperClass;
    _id = QMocSuperClass::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    }
    return _id;
}

// SIGNAL 0
void XtalOpt::Xtal::dimensionsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}
QT_END_MOC_NAMESPACE
