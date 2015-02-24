/****************************************************************************
** Meta object code from reading C++ file 'extension.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.6)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/randomdock/extension.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'extension.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.6. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_RandomDock__RandomDockExtension[] = {

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
      32,   80,   90,   90, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_RandomDock__RandomDockExtension[] = {
    "RandomDock::RandomDockExtension\0"
    "reemitMoleculeChanged(GlobalSearch::Structure*)\0"
    "structure\0\0"
};

void RandomDock::RandomDockExtension::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        RandomDockExtension *_t = static_cast<RandomDockExtension *>(_o);
        switch (_id) {
        case 0: _t->reemitMoleculeChanged((*reinterpret_cast< GlobalSearch::Structure*(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData RandomDock::RandomDockExtension::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject RandomDock::RandomDockExtension::staticMetaObject = {
    { &Avogadro::Extension::staticMetaObject, qt_meta_stringdata_RandomDock__RandomDockExtension,
      qt_meta_data_RandomDock__RandomDockExtension, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &RandomDock::RandomDockExtension::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *RandomDock::RandomDockExtension::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *RandomDock::RandomDockExtension::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_RandomDock__RandomDockExtension))
        return static_cast<void*>(const_cast< RandomDockExtension*>(this));
    typedef Avogadro::Extension QMocSuperClass;
    return QMocSuperClass::qt_metacast(_clname);
}

int RandomDock::RandomDockExtension::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    typedef Avogadro::Extension QMocSuperClass;
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
static const uint qt_meta_data_RandomDock__RandomDockExtensionFactory[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

static const char qt_meta_stringdata_RandomDock__RandomDockExtensionFactory[] = {
    "RandomDock::RandomDockExtensionFactory\0"
};

void RandomDock::RandomDockExtensionFactory::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObjectExtraData RandomDock::RandomDockExtensionFactory::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject RandomDock::RandomDockExtensionFactory::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_RandomDock__RandomDockExtensionFactory,
      qt_meta_data_RandomDock__RandomDockExtensionFactory, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &RandomDock::RandomDockExtensionFactory::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *RandomDock::RandomDockExtensionFactory::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *RandomDock::RandomDockExtensionFactory::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_RandomDock__RandomDockExtensionFactory))
        return static_cast<void*>(const_cast< RandomDockExtensionFactory*>(this));
    if (!strcmp(_clname, "Avogadro::PluginFactory"))
        return static_cast< Avogadro::PluginFactory*>(const_cast< RandomDockExtensionFactory*>(this));
    if (!strcmp(_clname, "net.sourceforge.avogadro.pluginfactory/1.5"))
        return static_cast< Avogadro::PluginFactory*>(const_cast< RandomDockExtensionFactory*>(this));
    return QObject::qt_metacast(_clname);
}

int RandomDock::RandomDockExtensionFactory::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
QT_END_MOC_NAMESPACE
