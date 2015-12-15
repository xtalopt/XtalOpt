
#include <vector>

#include <QCheckBox>
#include <QtCore/QDebug>

#include "spgInitDialog.h"
#include "spgInit.h"

#include <xtalopt/xtalopt.h>

#include <openbabel/math/spacegroup.h>

namespace XtalOpt {

  SpgInitDialog::SpgInitDialog(XtalOpt* p, QWidget* parent) :
    QDialog(parent),
    m_xtalopt(p)
  {
    setupUi(this);

    QHash<uint, XtalCompositionStruct> comp = m_xtalopt->comp;

    QList<uint> atomicNums = comp.keys();
    QList<uint> FUList = m_xtalopt->formulaUnitsList;

    QList<uint> atoms;
    for (size_t i = 0; i < atomicNums.size(); i++) {
      for (size_t j = 0; j < comp.value(atomicNums[i]).quantity; j++) {
        atoms.push_back(atomicNums[i]);
      }
    }

    // Let's investigate every spacegroup!
    for (size_t spg = 1; spg <= 230; spg++) {
      uint index = spg - 1;
      QString FUPossible = "";
      // Let's also investigate every formula unit possible!
      for (size_t i = 0; i < FUList.size(); i++) {
        qDebug() << "Testing for FU" << QString::number(FUList.at(i));
        // Make an atoms list that is adjusted by the formula units
        std::vector<uint> tempAtoms;
        tempAtoms.reserve(atoms.size() * FUList.at(i));
        for (size_t j = 0; j < atoms.size(); j++) {
          for (size_t k = 0; k < FUList.at(i); k++) {
            qDebug() << "pushing back tempAtoms with " << QString::number(atoms.at(j));
            tempAtoms.push_back(atoms.at(j));
          }
        }
        if (SpgInit::isSpgPossible(spg, tempAtoms)) {
          FUPossible.append(QString::number(FUList.at(i)) + ",");
          qDebug() << "For spg of " << QString::number(spg) << "and FU of"
                   << QString::number(FUList.at(i)) << "the spg is possible!";
        }
      }
      qDebug() << "FUPossible is " << FUPossible;
      // Remove the last comma
      if (FUPossible.at(FUPossible.size() - 1) == ',')
        FUPossible.remove(FUPossible.size() - 1, 1);
      // Add the new row
      this->table_list->insertRow(index);
      for (int i = 0; i < 4; i++)
        this->table_list->setItem(index, i, new QTableWidgetItem());
      Spg_Table_Entry e;

      // Openbabel crashes when you try to retrieve the HM for spg = 230
      // for some reason...
      if (spg != 230)
        e.HM_spg = QString::fromStdString(OpenBabel::SpaceGroup::GetSpaceGroup(
                                                          spg)->GetHMName());
      else e.HM_spg = "Ia-3d";
      e.formulaUnitsPossible = FUPossible;
      e.formulaUnitsAllowed = " ";
      e.minNumOfEach = 0;
      e.brush = QBrush(Qt::blue);
      setTableEntry(index, e);
//    emit infoUpdate();

    }

/*
    QTableWidget* table = new QTableWidget();
    table->setFixedSize(350,150);
    table->setWindowTitle("QTableWidget Add CheckBox to Table Cell");

    QPalette* palette = new QPalette();
    palette->setColor(QPalette::Highlight,Qt::cyan);
    table->setPalette(*palette);

    table->setRowCount(2);
    table->setColumnCount(3);

    //Set Header Label Texts Here
    table->setHorizontalHeaderLabels(QString("HEADER 1;HEADER 2;HEADER 3").split(";"));
    //Add Table items here
    table->setItem(0,0,new QTableWidgetItem("ITEM 1_1"));
    table->setItem(0,1,new QTableWidgetItem("ITEM 1_2"));
    table->setItem(0,2,new QTableWidgetItem("ITEM 1_3"));

    table->setCellWidget(1,1,new QCheckBox("Checkbox"));

    table->show();
*/
  }

  SpgInitDialog::~SpgInitDialog()
  {
  }

  void SpgInitDialog::setTableEntry(uint row, const Spg_Table_Entry& e)
  {
    this->table_list->item(row, HM_Spg)->setText(e.HM_spg);
    this->table_list->item(row, FormulaUnitsPossible)->setText(
                                                      e.formulaUnitsPossible);
    this->table_list->item(row, FormulaUnitsAllowed)->setText(
                                                      e.formulaUnitsAllowed);
//    this->table_list->item(row, MinNumOfEach)->setText(
  //                                           QString::number(e.minNumOfEach));
  }

/*
  QDialog* SpgInitDialog::dialog()
  {
    if (!m_dialog) {
      m_dialog = new SpgInitDialog(m_xtalopt);
    }
    SlurmConfigDialog *d = qobject_cast<SlurmConfigDialog*>(m_dialog);
    d->updateGUI();

    return d;
  }
*/
}
