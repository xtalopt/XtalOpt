/****************************************************************************
** Meta object code from reading C++ file 'dialog.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.6)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/xtalopt/ui/dialog.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'dialog.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.6. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_XtalOpt__XtalOptDialog[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      23,   37,   37,   37, 0x0a,
      38,   37,   37,   37, 0x0a,
      59,   37,   37,   37, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_XtalOpt__XtalOptDialog[] = {
    "XtalOpt::XtalOptDialog\0saveSession()\0"
    "\0showTutorialDialog()\0startSearch()\0"
};

void XtalOpt::XtalOptDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        XtalOptDialog *_t = static_cast<XtalOptDialog *>(_o);
        switch (_id) {
        case 0: _t->saveSession(); break;
        case 1: _t->showTutorialDialog(); break;
        case 2: _t->startSearch(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData XtalOpt::XtalOptDialog::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject XtalOpt::XtalOptDialog::staticMetaObject = {
    { &GlobalSearch::AbstractDialog::staticMetaObject, qt_meta_stringdata_XtalOpt__XtalOptDialog,
      qt_meta_data_XtalOpt__XtalOptDialog, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &XtalOpt::XtalOptDialog::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *XtalOpt::XtalOptDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *XtalOpt::XtalOptDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_XtalOpt__XtalOptDialog))
        return static_cast<void*>(const_cast< XtalOptDialog*>(this));
    typedef GlobalSearch::AbstractDialog QMocSuperClass;
    return QMocSuperClass::qt_metacast(_clname);
}

int XtalOpt::XtalOptDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    typedef GlobalSearch::AbstractDialog QMocSuperClass;
    _id = QMocSuperClass::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
