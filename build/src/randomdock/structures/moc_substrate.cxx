/****************************************************************************
** Meta object code from reading C++ file 'substrate.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.6)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/randomdock/structures/substrate.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'substrate.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.6. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_RandomDock__Substrate[] = {

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
      22,   33,   39,   46, 0x0a,
      47,   46,   46,   46, 0x0a,
      64,   46,   46,   46, 0x0a,
      88,   46,   46,   46, 0x0a,
     101,   46,  127,   46, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_RandomDock__Substrate[] = {
    "RandomDock::Substrate\0prob(uint)\0index\0"
    "double\0\0sortConformers()\0"
    "generateProbabilities()\0checkProbs()\0"
    "getRandomConformerIndex()\0int\0"
};

void RandomDock::Substrate::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        Substrate *_t = static_cast<Substrate *>(_o);
        switch (_id) {
        case 0: { double _r = _t->prob((*reinterpret_cast< uint(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< double*>(_a[0]) = _r; }  break;
        case 1: _t->sortConformers(); break;
        case 2: _t->generateProbabilities(); break;
        case 3: _t->checkProbs(); break;
        case 4: { int _r = _t->getRandomConformerIndex();
            if (_a[0]) *reinterpret_cast< int*>(_a[0]) = _r; }  break;
        default: ;
        }
    }
}

const QMetaObjectExtraData RandomDock::Substrate::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject RandomDock::Substrate::staticMetaObject = {
    { &GlobalSearch::Structure::staticMetaObject, qt_meta_stringdata_RandomDock__Substrate,
      qt_meta_data_RandomDock__Substrate, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &RandomDock::Substrate::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *RandomDock::Substrate::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *RandomDock::Substrate::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_RandomDock__Substrate))
        return static_cast<void*>(const_cast< Substrate*>(this));
    typedef GlobalSearch::Structure QMocSuperClass;
    return QMocSuperClass::qt_metacast(_clname);
}

int RandomDock::Substrate::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    typedef GlobalSearch::Structure QMocSuperClass;
    _id = QMocSuperClass::qt_metacall(_c, _id, _a);
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
