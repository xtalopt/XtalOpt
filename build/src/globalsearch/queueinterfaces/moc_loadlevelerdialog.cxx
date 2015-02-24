/****************************************************************************
** Meta object code from reading C++ file 'loadlevelerdialog.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.6)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/globalsearch/queueinterfaces/loadlevelerdialog.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'loadlevelerdialog.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.6. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_GlobalSearch__LoadLevelerConfigDialog[] = {

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
      38,   50,   50,   50, 0x0a,
      51,   50,   50,   50, 0x09,
      60,   50,   50,   50, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_GlobalSearch__LoadLevelerConfigDialog[] = {
    "GlobalSearch::LoadLevelerConfigDialog\0"
    "updateGUI()\0\0accept()\0reject()\0"
};

void GlobalSearch::LoadLevelerConfigDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        LoadLevelerConfigDialog *_t = static_cast<LoadLevelerConfigDialog *>(_o);
        switch (_id) {
        case 0: _t->updateGUI(); break;
        case 1: _t->accept(); break;
        case 2: _t->reject(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData GlobalSearch::LoadLevelerConfigDialog::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject GlobalSearch::LoadLevelerConfigDialog::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_GlobalSearch__LoadLevelerConfigDialog,
      qt_meta_data_GlobalSearch__LoadLevelerConfigDialog, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &GlobalSearch::LoadLevelerConfigDialog::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *GlobalSearch::LoadLevelerConfigDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *GlobalSearch::LoadLevelerConfigDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_GlobalSearch__LoadLevelerConfigDialog))
        return static_cast<void*>(const_cast< LoadLevelerConfigDialog*>(this));
    return QDialog::qt_metacast(_clname);
}

int GlobalSearch::LoadLevelerConfigDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
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
