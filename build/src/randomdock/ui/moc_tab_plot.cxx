/****************************************************************************
** Meta object code from reading C++ file 'tab_plot.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.6)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/randomdock/ui/tab_plot.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'tab_plot.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.6. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_RandomDock__TabPlot[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      15,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      20,   42,   51,   51, 0x0a,
      52,   51,   51,   51, 0x2a,
      67,   42,   51,   51, 0x0a,
      90,   51,   51,   51, 0x2a,
     106,   51,   51,   51, 0x0a,
     118,   51,   51,   51, 0x0a,
     134,  170,   51,   51, 0x0a,
     173,  170,   51,   51, 0x0a,
     209,   51,   51,   51, 0x0a,
     223,   51,   51,   51, 0x0a,
     236,   51,   51,   51, 0x0a,
     249,   51,   51,   51, 0x0a,
     264,   51,   51,   51, 0x0a,
     288,  318,   51,   51, 0x0a,
     324,  369,   51,   51, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_RandomDock__TabPlot[] = {
    "RandomDock::TabPlot\0readSettings(QString)\0"
    "filename\0\0readSettings()\0"
    "writeSettings(QString)\0writeSettings()\0"
    "updateGUI()\0disconnectGUI()\0"
    "lockClearAndSelectPoint(PlotPoint*)\0"
    "pp\0selectStructureFromPlot(PlotPoint*)\0"
    "refreshPlot()\0updatePlot()\0plotTrends()\0"
    "plotDistHist()\0populateStructureList()\0"
    "selectStructureFromIndex(int)\0index\0"
    "highlightStructure(GlobalSearch::Structure*)\0"
    "stucture\0"
};

void RandomDock::TabPlot::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        TabPlot *_t = static_cast<TabPlot *>(_o);
        switch (_id) {
        case 0: _t->readSettings((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->readSettings(); break;
        case 2: _t->writeSettings((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 3: _t->writeSettings(); break;
        case 4: _t->updateGUI(); break;
        case 5: _t->disconnectGUI(); break;
        case 6: _t->lockClearAndSelectPoint((*reinterpret_cast< PlotPoint*(*)>(_a[1]))); break;
        case 7: _t->selectStructureFromPlot((*reinterpret_cast< PlotPoint*(*)>(_a[1]))); break;
        case 8: _t->refreshPlot(); break;
        case 9: _t->updatePlot(); break;
        case 10: _t->plotTrends(); break;
        case 11: _t->plotDistHist(); break;
        case 12: _t->populateStructureList(); break;
        case 13: _t->selectStructureFromIndex((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 14: _t->highlightStructure((*reinterpret_cast< GlobalSearch::Structure*(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData RandomDock::TabPlot::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject RandomDock::TabPlot::staticMetaObject = {
    { &GlobalSearch::AbstractTab::staticMetaObject, qt_meta_stringdata_RandomDock__TabPlot,
      qt_meta_data_RandomDock__TabPlot, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &RandomDock::TabPlot::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *RandomDock::TabPlot::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *RandomDock::TabPlot::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_RandomDock__TabPlot))
        return static_cast<void*>(const_cast< TabPlot*>(this));
    typedef GlobalSearch::AbstractTab QMocSuperClass;
    return QMocSuperClass::qt_metacast(_clname);
}

int RandomDock::TabPlot::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    typedef GlobalSearch::AbstractTab QMocSuperClass;
    _id = QMocSuperClass::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
