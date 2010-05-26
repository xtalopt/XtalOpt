#include "../src/xtalopt/xtalopt.h"
#include <QApplication>

using namespace Avogadro;

QHash<QString, QString> getKeywordHash() {
  QHash<QString, QString> h;
  // These are set explicitly:
  h.insert("user1",	"first user string");
  h.insert("user2",	"second user string");
  h.insert("user3",	"third user string");
  h.insert("user4",	"fourth user string");
  h.insert("description","descriptive string");
  h.insert("a",	"1");
  h.insert("b",	"2");
  h.insert("c",	"3");
  h.insert("alphaDeg",	"90");
  h.insert("betaDeg",	"70");
  h.insert("gammaDeg",	"110");
  h.insert("numAtoms",	"2");
  h.insert("numSpecies","2");
  h.insert("filename",	"/tmp/localpath/");
  h.insert("rempath",	"/tmp/remotepath/");
  h.insert("gen",	"1");
  h.insert("id",	"3");
  h.insert("optStep",	"4");
  h.insert("coords",	"H 0 0 0\nO 1 2 3");
  h.insert("coordsId",	"H 1 0 0 0\nO 8 1 2 3");
  h.insert("coordsFrac",	"H 0 0 0\nO 0.480384 0.850833 1.07364");
  h.insert("coordsFracId",	"H 1 0 0 0\nO 8 0.480384 0.850833 1.07364");
  h.insert("cellMatrixAngstrom",	"1\t0\t0\n-0.68404\t1.87939\t0\n1.02606\t0.373455\t2.79423");
  h.insert("cellMatrixBohr",	"1.88973\t0\t0\n-1.29265\t3.55152\t0\n1.93897\t0.705728\t5.28033");
  h.insert("alphaRad",	"1.5708");
  h.insert("betaRad",	"1.22173");
  h.insert("gammaRad",	"1.91986");
  h.insert("volume",	"5.25144");
  h.insert("cellVector1Angstrom",	"1\t0\t0\t");
  h.insert("cellVector2Angstrom",	"-0.68404\t1.87939\t0\t");
  h.insert("cellVector3Angstrom",	"1.02606\t0.373455\t2.79423\t");
  h.insert("cellVector1Bohr",	"1.88973\t0\t0\t");
  h.insert("cellVector2Bohr",	"-1.29265\t3.55152\t0\t");
  h.insert("cellVector3Bohr",	"1.93897\t0.705728\t5.28033");
  h.insert("POSCAR",	"/tmp/localpath/\n1\n1 0 0 \n-0.68404 1.87939 0 \n1.02606 0.373455 2.79423 \n1 1 \nDirect\n0 0 0 \n0.480384 0.850833 1.07364 \n");
  return h;
}

bool near(double d1, double d2, double tol=1e-4) {
  if (fabs(d1 - d2) < tol) return true;
  else return false;
}

Xtal* getXtal(QHash<QString, QString> &hash) {
  Xtal *xtal  = new Xtal(hash["a"].toDouble(),
                         hash["b"].toDouble(),
                         hash["c"].toDouble(),
                         hash["alphaDeg"].toDouble(),
                         hash["betaDeg"].toDouble(),
                         hash["gammaDeg"].toDouble());
  Eigen::Vector3d pos1 (0,0,0);
  Eigen::Vector3d pos2 (1,2,3);

  Atom* atm1 = xtal->addAtom();
  atm1->setPos(pos1);
  atm1->setAtomicNumber(1);

  Atom* atm2 = xtal->addAtom();
  atm2->setPos(pos2);
  atm2->setAtomicNumber(8);

  xtal->setEnergy(100);
  xtal->setPV(50);
  xtal->setEnthalpy(150);

  xtal->setGeneration(hash["gen"].toUInt());
  xtal->setIDNumber(hash["id"].toUInt());
  xtal->setCurrentOptStep(hash["optStep"].toUInt());

  xtal->setFileName(hash["filename"]);
  xtal->setRempath(hash["rempath"]);
  return xtal;
}

void setupOptimizer(XtalOpt *xo, QHash<QString, QString> &hash) {
  Optimizer *opt = xo->optimizer();
  opt->setUser1(hash["user1"]);
  opt->setUser2(hash["user2"]);
  opt->setUser3(hash["user3"]);
  opt->setUser4(hash["user4"]);
}

void setupXtalOpt(XtalOpt *xo, QHash<QString, QString> &hash) {
  xo->setOptimizer("GULP");
  xo->description = hash["description"];
  setupOptimizer(xo, hash);
  xo->saveOnExit = false;
}

int main(int argc, char *argv[]) {
  QApplication qa (argc, argv);
  QHash<QString, QString> hash = getKeywordHash();
  XtalOptDialog dialog;
  XtalOpt *xo = dialog.getXtalOpt();
  Xtal *xtal = getXtal(hash);
  setupXtalOpt(xo, hash);

  QStringList keys = hash.keys();
  QString key, hashval,templ;
  bool ok = true;
  for (int i = 0; i < keys.size(); i++) {
    key = keys.at(i);
    templ = xo->interpretTemplate(QString("%%1%")
                                 .arg(key),
                                 xtal);
    hashval = hash[key];
    if (templ.simplified().localeAwareCompare(hashval.simplified()) != 0) {
      qDebug() << QString("%1 did not convert correctly:\nExpected:\n%2\n\nGot:\n%3\n")
        .arg(key).arg(hashval).arg(templ);
      ok = false;
    }
  }

  // Clean up
  delete xtal;
  if (ok) return 0;
  else return 1;
}
