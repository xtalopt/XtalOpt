/****************************************************************************
** Meta object code from reading C++ file 'abstracttab.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.6)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/globalsearch/ui/abstracttab.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'abstracttab.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.6. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_GlobalSearch__AbstractTab[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      14,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: signature, parameters, type, tag, flags
      26,   68,   68,   68, 0x05,
      69,   68,   68,   68, 0x05,
     100,   68,   68,   68, 0x05,
     131,   68,   68,   68, 0x05,

 // slots: signature, parameters, type, tag, flags
     145,   68,   68,   68, 0x0a,
     155,  177,   68,   68, 0x0a,
     186,   68,   68,   68, 0x2a,
     201,  177,   68,   68, 0x0a,
     224,   68,   68,   68, 0x2a,
     240,   68,   68,   68, 0x0a,
     252,   68,   68,   68, 0x0a,
     268,   68,   68,   68, 0x09,
     281,   68,   68,   68, 0x09,
     297,   68,   68,   68, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_GlobalSearch__AbstractTab[] = {
    "GlobalSearch::AbstractTab\0"
    "moleculeChanged(GlobalSearch::Structure*)\0"
    "\0startingBackgroundProcessing()\0"
    "finishedBackgroundProcessing()\0"
    "initialized()\0lockGUI()\0readSettings(QString)\0"
    "filename\0readSettings()\0writeSettings(QString)\0"
    "writeSettings()\0updateGUI()\0disconnectGUI()\0"
    "initialize()\0setBusyCursor()\0"
    "clearBusyCursor()\0"
};

void GlobalSearch::AbstractTab::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        AbstractTab *_t = static_cast<AbstractTab *>(_o);
        switch (_id) {
        case 0: _t->moleculeChanged((*reinterpret_cast< GlobalSearch::Structure*(*)>(_a[1]))); break;
        case 1: _t->startingBackgroundProcessing(); break;
        case 2: _t->finishedBackgroundProcessing(); break;
        case 3: _t->initialized(); break;
        case 4: _t->lockGUI(); break;
        case 5: _t->readSettings((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 6: _t->readSettings(); break;
        case 7: _t->writeSettings((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 8: _t->writeSettings(); break;
        case 9: _t->updateGUI(); break;
        case 10: _t->disconnectGUI(); break;
        case 11: _t->initialize(); break;
        case 12: _t->setBusyCursor(); break;
        case 13: _t->clearBusyCursor(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData GlobalSearch::AbstractTab::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject GlobalSearch::AbstractTab::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_GlobalSearch__AbstractTab,
      qt_meta_data_GlobalSearch__AbstractTab, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &GlobalSearch::AbstractTab::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *GlobalSearch::AbstractTab::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *GlobalSearch::AbstractTab::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_GlobalSearch__AbstractTab))
        return static_cast<void*>(const_cast< AbstractTab*>(this));
    return QObject::qt_metacast(_clname);
}

int GlobalSearch::AbstractTab::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 14)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 14;
    }
    return _id;
}

// SIGNAL 0
void GlobalSearch::AbstractTab::moleculeChanged(GlobalSearch::Structure * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void GlobalSearch::AbstractTab::startingBackgroundProcessing()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}

// SIGNAL 2
void GlobalSearch::AbstractTab::finishedBackgroundProcessing()
{
    QMetaObject::activate(this, &staticMetaObject, 2, 0);
}

// SIGNAL 3
void GlobalSearch::AbstractTab::initialized()
{
    QMetaObject::activate(this, &staticMetaObject, 3, 0);
}
QT_END_MOC_NAMESPACE
