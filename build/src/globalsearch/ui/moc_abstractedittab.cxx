/****************************************************************************
** Meta object code from reading C++ file 'abstractedittab.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.6)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/globalsearch/ui/abstractedittab.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'abstractedittab.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.6. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_GlobalSearch__AbstractEditTab[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      20,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: signature, parameters, type, tag, flags
      30,   59,   59,   59, 0x05,
      60,   59,   59,   59, 0x05,

 // slots: signature, parameters, type, tag, flags
      99,   59,   59,   59, 0x0a,
     109,   59,   59,   59, 0x0a,
     121,   59,   59,   59, 0x0a,
     140,   59,   59,   59, 0x0a,
     151,   59,   59,   59, 0x0a,
     173,   59,   59,   59, 0x0a,
     195,   59,   59,   59, 0x0a,
     215,   59,   59,   59, 0x0a,
     231,   59,   59,   59, 0x0a,
     254,   59,   59,   59, 0x0a,
     267,   59,   59,   59, 0x0a,
     280,   59,  299,   59, 0x0a,
     311,   59,   59,   59, 0x09,
     324,   59,   59,   59, 0x09,
     343,   59,   59,   59, 0x09,
     366,   59,   59,   59, 0x09,
     384,   59,   59,   59, 0x09,
     410,   59,   59,   59, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_GlobalSearch__AbstractEditTab[] = {
    "GlobalSearch::AbstractEditTab\0"
    "optimizerChanged(Optimizer*)\0\0"
    "queueInterfaceChanged(QueueInterface*)\0"
    "lockGUI()\0updateGUI()\0updateEditWidget()\0"
    "showHelp()\0saveCurrentTemplate()\0"
    "populateOptStepList()\0populateTemplates()\0"
    "appendOptStep()\0removeCurrentOptStep()\0"
    "saveScheme()\0loadScheme()\0getTemplateNames()\0"
    "QStringList\0initialize()\0updateUserValues()\0"
    "updateQueueInterface()\0updateOptimizer()\0"
    "configureQueueInterface()\0"
    "configureOptimizer()\0"
};

void GlobalSearch::AbstractEditTab::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        AbstractEditTab *_t = static_cast<AbstractEditTab *>(_o);
        switch (_id) {
        case 0: _t->optimizerChanged((*reinterpret_cast< Optimizer*(*)>(_a[1]))); break;
        case 1: _t->queueInterfaceChanged((*reinterpret_cast< QueueInterface*(*)>(_a[1]))); break;
        case 2: _t->lockGUI(); break;
        case 3: _t->updateGUI(); break;
        case 4: _t->updateEditWidget(); break;
        case 5: _t->showHelp(); break;
        case 6: _t->saveCurrentTemplate(); break;
        case 7: _t->populateOptStepList(); break;
        case 8: _t->populateTemplates(); break;
        case 9: _t->appendOptStep(); break;
        case 10: _t->removeCurrentOptStep(); break;
        case 11: _t->saveScheme(); break;
        case 12: _t->loadScheme(); break;
        case 13: { QStringList _r = _t->getTemplateNames();
            if (_a[0]) *reinterpret_cast< QStringList*>(_a[0]) = _r; }  break;
        case 14: _t->initialize(); break;
        case 15: _t->updateUserValues(); break;
        case 16: _t->updateQueueInterface(); break;
        case 17: _t->updateOptimizer(); break;
        case 18: _t->configureQueueInterface(); break;
        case 19: _t->configureOptimizer(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData GlobalSearch::AbstractEditTab::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject GlobalSearch::AbstractEditTab::staticMetaObject = {
    { &GlobalSearch::AbstractTab::staticMetaObject, qt_meta_stringdata_GlobalSearch__AbstractEditTab,
      qt_meta_data_GlobalSearch__AbstractEditTab, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &GlobalSearch::AbstractEditTab::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *GlobalSearch::AbstractEditTab::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *GlobalSearch::AbstractEditTab::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_GlobalSearch__AbstractEditTab))
        return static_cast<void*>(const_cast< AbstractEditTab*>(this));
    typedef GlobalSearch::AbstractTab QMocSuperClass;
    return QMocSuperClass::qt_metacast(_clname);
}

int GlobalSearch::AbstractEditTab::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
void GlobalSearch::AbstractEditTab::optimizerChanged(Optimizer * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void GlobalSearch::AbstractEditTab::queueInterfaceChanged(QueueInterface * _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_END_MOC_NAMESPACE
