/****************************************************************************
** Meta object code from reading C++ file 'structure.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.6)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/globalsearch/structure.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'structure.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.6. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_GlobalSearch__Structure[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      48,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      24,   43,   43,   43, 0x0a,
      44,   43,   43,   43, 0x0a,
      80,   43,   43,   43, 0x0a,
     109,   43,   43,   43, 0x0a,
     136,   43,   43,   43, 0x0a,
     155,  262,  288,   43, 0x0a,
     293,  262,  288,   43, 0x0a,
     382,  262,  288,   43, 0x0a,
     477,  500,   43,   43, 0x0a,
     509,  500,   43,   43, 0x0a,
     531,  618,   43,   43, 0x0a,
     657,  728,   43,   43, 0x2a,
     762,  826,   43,   43, 0x2a,
     851,  908,   43,   43, 0x2a,
     926,  618,   43,   43, 0x0a,
    1014,  728,   43,   43, 0x2a,
    1086,  826,   43,   43, 0x2a,
    1151,  908,   43,   43, 0x2a,
    1209, 1233,   43,   43, 0x0a,
    1239, 1336,   43,   43, 0x0a,
    1381,   43, 1397,   43, 0x0a,
    1402, 1422,   43,   43, 0x0a,
    1431, 1445,   43,   43, 0x0a,
    1448,   43,   43,   43, 0x0a,
    1464,   43,   43,   43, 0x0a,
    1478, 1499,   43,   43, 0x0a,
    1502,   43,   43,   43, 0x2a,
    1516, 1530,   43,   43, 0x0a,
    1535, 1550,   43,   43, 0x0a,
    1553, 1573,   43,   43, 0x0a,
    1577, 1550,   43,   43, 0x0a,
    1595, 1233,   43,   43, 0x0a,
    1609, 1629,   43,   43, 0x0a,
    1631, 1629,   43,   43, 0x0a,
    1651, 1668,   43,   43, 0x0a,
    1675, 1699,   43,   43, 0x0a,
    1701, 1720,   43,   43, 0x0a,
    1726,   43,   43,   43, 0x0a,
    1743,   43,   43,   43, 0x0a,
    1756, 1784,   43,   43, 0x0a,
    1786, 1818,   43,   43, 0x0a,
    1820,   43,   43,   43, 0x0a,
    1836,   43,   43,   43, 0x0a,
    1851, 1879,   43,   43, 0x0a,
    1881, 1879,   43,   43, 0x0a,
    1907, 1926,   43,   43, 0x0a,
    1929,  500,   43,   43, 0x09,
    1961,  500,   43,   43, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_GlobalSearch__Structure[] = {
    "GlobalSearch::Structure\0setupConnections()\0"
    "\0enableAutoHistogramGeneration(bool)\0"
    "requestHistogramGeneration()\0"
    "generateDefaultHistogram()\0"
    "structureChanged()\0"
    "compareIADDistributions(std::vector<double>,std::vector<double>,std::v"
    "ector<double>,double,double,double*)\0"
    "d,f1,f2,decay,smear,error\0bool\0"
    "compareIADDistributions(QList<double>,QList<double>,QList<double>,doub"
    "le,double,double*)\0"
    "compareIADDistributions(QList<QVariant>,QList<QVariant>,QList<QVariant"
    ">,double,double,double*)\0"
    "writeSettings(QString)\0filename\0"
    "readSettings(QString)\0"
    "updateAndSkipHistory(QList<uint>,QList<Eigen::Vector3d>,double,double,"
    "Eigen::Matrix3d)\0"
    "atomicNums,coords,energy,enthalpy,cell\0"
    "updateAndSkipHistory(QList<uint>,QList<Eigen::Vector3d>,double,double)\0"
    "atomicNums,coords,energy,enthalpy\0"
    "updateAndSkipHistory(QList<uint>,QList<Eigen::Vector3d>,double)\0"
    "atomicNums,coords,energy\0"
    "updateAndSkipHistory(QList<uint>,QList<Eigen::Vector3d>)\0"
    "atomicNums,coords\0"
    "updateAndAddToHistory(QList<uint>,QList<Eigen::Vector3d>,double,double"
    ",Eigen::Matrix3d)\0"
    "updateAndAddToHistory(QList<uint>,QList<Eigen::Vector3d>,double,double"
    ")\0"
    "updateAndAddToHistory(QList<uint>,QList<Eigen::Vector3d>,double)\0"
    "updateAndAddToHistory(QList<uint>,QList<Eigen::Vector3d>)\0"
    "deleteFromHistory(uint)\0index\0"
    "retrieveHistoryEntry(uint,QList<uint>*,QList<Eigen::Vector3d>*,double*"
    ",double*,Eigen::Matrix3d*)\0"
    "index,atomicNums,coords,energy,enthalpy,cell\0"
    "sizeOfHistory()\0uint\0setEnthalpy(double)\0"
    "enthalpy\0setPV(double)\0pv\0resetEnthalpy()\0"
    "resetEnergy()\0setOBEnergy(QString)\0"
    "ff\0setOBEnergy()\0setRank(uint)\0rank\0"
    "setJobID(uint)\0id\0setGeneration(uint)\0"
    "gen\0setIDNumber(uint)\0setIndex(int)\0"
    "setParents(QString)\0p\0setRempath(QString)\0"
    "setStatus(State)\0status\0setCurrentOptStep(uint)\0"
    "i\0setFailCount(uint)\0count\0resetFailCount()\0"
    "addFailure()\0setDuplicateString(QString)\0"
    "s\0setChangedSinceDupChecked(bool)\0b\0"
    "startOptTimer()\0stopOptTimer()\0"
    "setOptTimerStart(QDateTime)\0d\0"
    "setOptTimerEnd(QDateTime)\0load(QTextStream&)\0"
    "in\0writeStructureSettings(QString)\0"
    "readStructureSettings(QString)\0"
};

void GlobalSearch::Structure::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        Structure *_t = static_cast<Structure *>(_o);
        switch (_id) {
        case 0: _t->setupConnections(); break;
        case 1: _t->enableAutoHistogramGeneration((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 2: _t->requestHistogramGeneration(); break;
        case 3: _t->generateDefaultHistogram(); break;
        case 4: _t->structureChanged(); break;
        case 5: { bool _r = _t->compareIADDistributions((*reinterpret_cast< const std::vector<double>(*)>(_a[1])),(*reinterpret_cast< const std::vector<double>(*)>(_a[2])),(*reinterpret_cast< const std::vector<double>(*)>(_a[3])),(*reinterpret_cast< double(*)>(_a[4])),(*reinterpret_cast< double(*)>(_a[5])),(*reinterpret_cast< double*(*)>(_a[6])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 6: { bool _r = _t->compareIADDistributions((*reinterpret_cast< const QList<double>(*)>(_a[1])),(*reinterpret_cast< const QList<double>(*)>(_a[2])),(*reinterpret_cast< const QList<double>(*)>(_a[3])),(*reinterpret_cast< double(*)>(_a[4])),(*reinterpret_cast< double(*)>(_a[5])),(*reinterpret_cast< double*(*)>(_a[6])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 7: { bool _r = _t->compareIADDistributions((*reinterpret_cast< const QList<QVariant>(*)>(_a[1])),(*reinterpret_cast< const QList<QVariant>(*)>(_a[2])),(*reinterpret_cast< const QList<QVariant>(*)>(_a[3])),(*reinterpret_cast< double(*)>(_a[4])),(*reinterpret_cast< double(*)>(_a[5])),(*reinterpret_cast< double*(*)>(_a[6])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 8: _t->writeSettings((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 9: _t->readSettings((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 10: _t->updateAndSkipHistory((*reinterpret_cast< const QList<uint>(*)>(_a[1])),(*reinterpret_cast< const QList<Eigen::Vector3d>(*)>(_a[2])),(*reinterpret_cast< const double(*)>(_a[3])),(*reinterpret_cast< const double(*)>(_a[4])),(*reinterpret_cast< const Eigen::Matrix3d(*)>(_a[5]))); break;
        case 11: _t->updateAndSkipHistory((*reinterpret_cast< const QList<uint>(*)>(_a[1])),(*reinterpret_cast< const QList<Eigen::Vector3d>(*)>(_a[2])),(*reinterpret_cast< const double(*)>(_a[3])),(*reinterpret_cast< const double(*)>(_a[4]))); break;
        case 12: _t->updateAndSkipHistory((*reinterpret_cast< const QList<uint>(*)>(_a[1])),(*reinterpret_cast< const QList<Eigen::Vector3d>(*)>(_a[2])),(*reinterpret_cast< const double(*)>(_a[3]))); break;
        case 13: _t->updateAndSkipHistory((*reinterpret_cast< const QList<uint>(*)>(_a[1])),(*reinterpret_cast< const QList<Eigen::Vector3d>(*)>(_a[2]))); break;
        case 14: _t->updateAndAddToHistory((*reinterpret_cast< const QList<uint>(*)>(_a[1])),(*reinterpret_cast< const QList<Eigen::Vector3d>(*)>(_a[2])),(*reinterpret_cast< const double(*)>(_a[3])),(*reinterpret_cast< const double(*)>(_a[4])),(*reinterpret_cast< const Eigen::Matrix3d(*)>(_a[5]))); break;
        case 15: _t->updateAndAddToHistory((*reinterpret_cast< const QList<uint>(*)>(_a[1])),(*reinterpret_cast< const QList<Eigen::Vector3d>(*)>(_a[2])),(*reinterpret_cast< const double(*)>(_a[3])),(*reinterpret_cast< const double(*)>(_a[4]))); break;
        case 16: _t->updateAndAddToHistory((*reinterpret_cast< const QList<uint>(*)>(_a[1])),(*reinterpret_cast< const QList<Eigen::Vector3d>(*)>(_a[2])),(*reinterpret_cast< const double(*)>(_a[3]))); break;
        case 17: _t->updateAndAddToHistory((*reinterpret_cast< const QList<uint>(*)>(_a[1])),(*reinterpret_cast< const QList<Eigen::Vector3d>(*)>(_a[2]))); break;
        case 18: _t->deleteFromHistory((*reinterpret_cast< uint(*)>(_a[1]))); break;
        case 19: _t->retrieveHistoryEntry((*reinterpret_cast< uint(*)>(_a[1])),(*reinterpret_cast< QList<uint>*(*)>(_a[2])),(*reinterpret_cast< QList<Eigen::Vector3d>*(*)>(_a[3])),(*reinterpret_cast< double*(*)>(_a[4])),(*reinterpret_cast< double*(*)>(_a[5])),(*reinterpret_cast< Eigen::Matrix3d*(*)>(_a[6]))); break;
        case 20: { uint _r = _t->sizeOfHistory();
            if (_a[0]) *reinterpret_cast< uint*>(_a[0]) = _r; }  break;
        case 21: _t->setEnthalpy((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 22: _t->setPV((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 23: _t->resetEnthalpy(); break;
        case 24: _t->resetEnergy(); break;
        case 25: _t->setOBEnergy((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 26: _t->setOBEnergy(); break;
        case 27: _t->setRank((*reinterpret_cast< uint(*)>(_a[1]))); break;
        case 28: _t->setJobID((*reinterpret_cast< uint(*)>(_a[1]))); break;
        case 29: _t->setGeneration((*reinterpret_cast< uint(*)>(_a[1]))); break;
        case 30: _t->setIDNumber((*reinterpret_cast< uint(*)>(_a[1]))); break;
        case 31: _t->setIndex((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 32: _t->setParents((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 33: _t->setRempath((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 34: _t->setStatus((*reinterpret_cast< State(*)>(_a[1]))); break;
        case 35: _t->setCurrentOptStep((*reinterpret_cast< uint(*)>(_a[1]))); break;
        case 36: _t->setFailCount((*reinterpret_cast< uint(*)>(_a[1]))); break;
        case 37: _t->resetFailCount(); break;
        case 38: _t->addFailure(); break;
        case 39: _t->setDuplicateString((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 40: _t->setChangedSinceDupChecked((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 41: _t->startOptTimer(); break;
        case 42: _t->stopOptTimer(); break;
        case 43: _t->setOptTimerStart((*reinterpret_cast< const QDateTime(*)>(_a[1]))); break;
        case 44: _t->setOptTimerEnd((*reinterpret_cast< const QDateTime(*)>(_a[1]))); break;
        case 45: _t->load((*reinterpret_cast< QTextStream(*)>(_a[1]))); break;
        case 46: _t->writeStructureSettings((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 47: _t->readStructureSettings((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData GlobalSearch::Structure::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject GlobalSearch::Structure::staticMetaObject = {
    { &Avogadro::Molecule::staticMetaObject, qt_meta_stringdata_GlobalSearch__Structure,
      qt_meta_data_GlobalSearch__Structure, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &GlobalSearch::Structure::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *GlobalSearch::Structure::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *GlobalSearch::Structure::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_GlobalSearch__Structure))
        return static_cast<void*>(const_cast< Structure*>(this));
    typedef Avogadro::Molecule QMocSuperClass;
    return QMocSuperClass::qt_metacast(_clname);
}

int GlobalSearch::Structure::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    typedef Avogadro::Molecule QMocSuperClass;
    _id = QMocSuperClass::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 48)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 48;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
