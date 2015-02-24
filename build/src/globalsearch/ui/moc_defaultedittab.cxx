/****************************************************************************
** Meta object code from reading C++ file 'defaultedittab.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.6)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/globalsearch/ui/defaultedittab.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'defaultedittab.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.6. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_GlobalSearch__DefaultEditTab[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      29,   42,   42,   42, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_GlobalSearch__DefaultEditTab[] = {
    "GlobalSearch::DefaultEditTab\0initialize()\0"
    "\0"
};

void GlobalSearch::DefaultEditTab::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        DefaultEditTab *_t = static_cast<DefaultEditTab *>(_o);
        switch (_id) {
        case 0: _t->initialize(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData GlobalSearch::DefaultEditTab::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject GlobalSearch::DefaultEditTab::staticMetaObject = {
    { &GlobalSearch::AbstractEditTab::staticMetaObject, qt_meta_stringdata_GlobalSearch__DefaultEditTab,
      qt_meta_data_GlobalSearch__DefaultEditTab, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &GlobalSearch::DefaultEditTab::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *GlobalSearch::DefaultEditTab::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *GlobalSearch::DefaultEditTab::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_GlobalSearch__DefaultEditTab))
        return static_cast<void*>(const_cast< DefaultEditTab*>(this));
    typedef GlobalSearch::AbstractEditTab QMocSuperClass;
    return QMocSuperClass::qt_metacast(_clname);
}

int GlobalSearch::DefaultEditTab::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    typedef GlobalSearch::AbstractEditTab QMocSuperClass;
    _id = QMocSuperClass::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
