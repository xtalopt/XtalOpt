/****************************************************************************
** Meta object code from reading C++ file 'xtalopttest.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.6)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/xtalopt/testing/xtalopttest.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'xtalopttest.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.6. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_XtalOpt__XtalOptTest[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: signature, parameters, type, tag, flags
      21,   36,   36,   36, 0x05,
      37,   36,   36,   36, 0x05,
      57,   36,   36,   36, 0x05,
      66,   36,   36,   36, 0x05,

 // slots: signature, parameters, type, tag, flags
      93,   36,   36,   36, 0x0a,
     116,   36,   36,   36, 0x0a,
     131,   36,   36,   36, 0x0a,
     154,  200,   36,   36, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_XtalOpt__XtalOptTest[] = {
    "XtalOpt::XtalOptTest\0testStarting()\0"
    "\0newMessage(QString)\0status()\0"
    "sig_updateProgressDialog()\0"
    "updateMessage(QString)\0updateStatus()\0"
    "updateProgressDialog()\0"
    "outputStatus(QString,int,int,int,int,int,int)\0"
    ",,,,,,\0"
};

void XtalOpt::XtalOptTest::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        XtalOptTest *_t = static_cast<XtalOptTest *>(_o);
        switch (_id) {
        case 0: _t->testStarting(); break;
        case 1: _t->newMessage((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 2: _t->status(); break;
        case 3: _t->sig_updateProgressDialog(); break;
        case 4: _t->updateMessage((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 5: _t->updateStatus(); break;
        case 6: _t->updateProgressDialog(); break;
        case 7: _t->outputStatus((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4])),(*reinterpret_cast< int(*)>(_a[5])),(*reinterpret_cast< int(*)>(_a[6])),(*reinterpret_cast< int(*)>(_a[7]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData XtalOpt::XtalOptTest::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject XtalOpt::XtalOptTest::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_XtalOpt__XtalOptTest,
      qt_meta_data_XtalOpt__XtalOptTest, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &XtalOpt::XtalOptTest::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *XtalOpt::XtalOptTest::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *XtalOpt::XtalOptTest::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_XtalOpt__XtalOptTest))
        return static_cast<void*>(const_cast< XtalOptTest*>(this));
    return QObject::qt_metacast(_clname);
}

int XtalOpt::XtalOptTest::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void XtalOpt::XtalOptTest::testStarting()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}

// SIGNAL 1
void XtalOpt::XtalOptTest::newMessage(const QString & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void XtalOpt::XtalOptTest::status()
{
    QMetaObject::activate(this, &staticMetaObject, 2, 0);
}

// SIGNAL 3
void XtalOpt::XtalOptTest::sig_updateProgressDialog()
{
    QMetaObject::activate(this, &staticMetaObject, 3, 0);
}
QT_END_MOC_NAMESPACE
