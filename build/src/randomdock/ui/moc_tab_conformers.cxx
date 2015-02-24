/****************************************************************************
** Meta object code from reading C++ file 'tab_conformers.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.6)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/randomdock/ui/tab_conformers.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'tab_conformers.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.6. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_RandomDock__TabConformers[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      20,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: signature, parameters, type, tag, flags
      26,   56,   56,   56, 0x05,
      57,   56,   56,   56, 0x05,
      83,   56,   56,   56, 0x05,

 // slots: signature, parameters, type, tag, flags
     103,   56,   56,   56, 0x0a,
     113,  135,   56,   56, 0x0a,
     144,   56,   56,   56, 0x2a,
     159,  135,   56,   56, 0x0a,
     182,   56,   56,   56, 0x2a,
     198,   56,   56,   56, 0x0a,
     210,   56,   56,   56, 0x0a,
     231,   56,   56,   56, 0x0a,
     253,   56,   56,   56, 0x0a,
     275,   56,   56,   56, 0x0a,
     298,  324,   56,   56, 0x0a,
     326,   56,   56,   56, 0x2a,
     345,  370,   56,   56, 0x0a,
     375,  409,   56,   56, 0x0a,
     422,  456,   56,   56, 0x0a,
     469,   56,   56,   56, 0x08,
     492,   56,   56,   56, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_RandomDock__TabConformers[] = {
    "RandomDock::TabConformers\0"
    "conformerGenerationStarting()\0\0"
    "conformerGenerationDone()\0conformersChanged()\0"
    "lockGUI()\0readSettings(QString)\0"
    "filename\0readSettings()\0writeSettings(QString)\0"
    "writeSettings()\0updateGUI()\0"
    "generateConformers()\0fillForceFieldCombo()\0"
    "updateStructureList()\0updateConformerTable()\0"
    "updateForceField(QString)\0s\0"
    "updateForceField()\0selectStructure(QString)\0"
    "text\0conformerChanged(int,int,int,int)\0"
    "row,,oldrow,\0calculateNumberOfConformers(bool)\0"
    "isSystematic\0enableGenerateButton()\0"
    "disableGenerateButton()\0"
};

void RandomDock::TabConformers::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        TabConformers *_t = static_cast<TabConformers *>(_o);
        switch (_id) {
        case 0: _t->conformerGenerationStarting(); break;
        case 1: _t->conformerGenerationDone(); break;
        case 2: _t->conformersChanged(); break;
        case 3: _t->lockGUI(); break;
        case 4: _t->readSettings((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 5: _t->readSettings(); break;
        case 6: _t->writeSettings((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 7: _t->writeSettings(); break;
        case 8: _t->updateGUI(); break;
        case 9: _t->generateConformers(); break;
        case 10: _t->fillForceFieldCombo(); break;
        case 11: _t->updateStructureList(); break;
        case 12: _t->updateConformerTable(); break;
        case 13: _t->updateForceField((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 14: _t->updateForceField(); break;
        case 15: _t->selectStructure((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 16: _t->conformerChanged((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4]))); break;
        case 17: _t->calculateNumberOfConformers((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 18: _t->enableGenerateButton(); break;
        case 19: _t->disableGenerateButton(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData RandomDock::TabConformers::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject RandomDock::TabConformers::staticMetaObject = {
    { &GlobalSearch::AbstractTab::staticMetaObject, qt_meta_stringdata_RandomDock__TabConformers,
      qt_meta_data_RandomDock__TabConformers, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &RandomDock::TabConformers::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *RandomDock::TabConformers::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *RandomDock::TabConformers::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_RandomDock__TabConformers))
        return static_cast<void*>(const_cast< TabConformers*>(this));
    typedef GlobalSearch::AbstractTab QMocSuperClass;
    return QMocSuperClass::qt_metacast(_clname);
}

int RandomDock::TabConformers::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    typedef GlobalSearch::AbstractTab QMocSuperClass;
    _id = QMocSuperClass::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 20)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 20;
    }
    return _id;
}

// SIGNAL 0
void RandomDock::TabConformers::conformerGenerationStarting()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}

// SIGNAL 1
void RandomDock::TabConformers::conformerGenerationDone()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}

// SIGNAL 2
void RandomDock::TabConformers::conformersChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, 0);
}
QT_END_MOC_NAMESPACE
