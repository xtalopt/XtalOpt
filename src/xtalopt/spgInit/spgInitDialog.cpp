
#include <vector>

#include <QCheckBox>
#include <QSpinBox>
#include <QtCore/QDebug>

#include "spgInitDialog.h"
#include "spgInit.h"

#include <xtalopt/xtalopt.h>

#include <globalsearch/fileutils.h>

#include <openbabel/math/spacegroup.h>

namespace XtalOpt {

  SpgInitDialog::SpgInitDialog(XtalOpt* p, QWidget* parent) :
    QDialog(parent),
    m_xtalopt(p),
    m_checkBoxList(QList<QCheckBox*>()),
    m_spinBoxList(QList<QSpinBox*>())
  {
    // Since SpgInitDialog inherits from the qt-created class, Ui::SpgInitDialog
    // We can just tell it to set up itself
    setupUi(this);

    // Make connections
    connect(this->push_selectAll, SIGNAL(clicked()),
            this, SLOT(selectAll()));
    connect(this->push_deselectAll, SIGNAL(clicked()),
            this, SLOT(deselectAll()));

    // Get the list of atoms
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
        // Make an atoms list that is adjusted by the formula units
        std::vector<uint> tempAtoms;
        tempAtoms.reserve(atoms.size() * FUList.at(i));
        for (size_t j = 0; j < atoms.size(); j++) {
          for (size_t k = 0; k < FUList.at(i); k++) {
            tempAtoms.push_back(atoms.at(j));
          }
        }
        // Append each formula unit to the list followed by a comma
        if (SpgInit::isSpgPossible(spg, tempAtoms)) {
          FUPossible.append(QString::number(FUList.at(i)) + ",");
        }
      }

      QString result = "";
      // This returns a list of unsigned integers. It returns empty
      // if it cannot be made successfully
      if (FileUtils::parseUIntString(FUPossible, result).size() != 0)
        FUPossible = result;

      else FUPossible = "";

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

      // Add a new checkbox
      m_checkBoxList.append(new QCheckBox);
      // Set the checkbox checked if the formula unit is possible
      if (FUPossible.size() != 0) m_checkBoxList.at(index)->setChecked(true);
      // If the spacegroup cannot be made, disable the checkbox
      else m_checkBoxList.at(index)->setEnabled(false);

      // This starts off disabled...
      m_spinBoxList.append(getNewSpinBox());
      if (FUPossible.size() != 0) m_spinBoxList.at(index)->setEnabled(true);

      e.formulaUnitsPossible = FUPossible;
      e.formulaUnitsAllowed = " ";
      e.minNumOfEach = 0;
      e.brush = QBrush(Qt::green);
      setTableEntry(index, e);
    }

  }

  SpgInitDialog::~SpgInitDialog()
  {
    // Delete the dynamically allocated checkbox list
    for (size_t i = 0; i < m_checkBoxList.size(); i++) {
      if (m_checkBoxList.at(i)) {
        delete m_checkBoxList.at(i);
        m_checkBoxList[i] = 0;
      }
    }

    for (size_t i = 0; i < m_spinBoxList.size(); i++) {
      if (m_spinBoxList.at(i)) {
        delete m_spinBoxList.at(i);
        m_spinBoxList[i] = 0;
      }
    }
  }

  void SpgInitDialog::setTableEntry(uint row, const Spg_Table_Entry& e)
  {
    this->table_list->item(row, HM_Spg)->setText(e.HM_spg);
    this->table_list->item(row, FormulaUnitsPossible)->setText(
                                                      e.formulaUnitsPossible);
    this->table_list->setCellWidget(row,FormulaUnitsAllowed, m_checkBoxList.at(row));
    this->table_list->setCellWidget(row, MinNumOfEach, m_spinBoxList.at(row));
//    this->table_list->item(row, FormulaUnitsAllowed)->setText(
//                                                      e.formulaUnitsAllowed);
//    this->table_list->item(row, MinNumOfEach)->setText(
  //                                           QString::number(e.minNumOfEach));
  }

  void SpgInitDialog::selectAll()
  {
    for (size_t i = 0; i < m_checkBoxList.size(); i++) {
      if (m_checkBoxList.at(i)->isEnabled())
        m_checkBoxList.at(i)->setChecked(true);
    }
  }

  void SpgInitDialog::deselectAll()
  {
    for (size_t i = 0; i < m_checkBoxList.size(); i++) {
      if (m_checkBoxList.at(i)->isEnabled())
        m_checkBoxList.at(i)->setChecked(false);
    }
  }

  QSpinBox* SpgInitDialog::getNewSpinBox()
  {
    QSpinBox* spinBox = new QSpinBox;
    spinBox->setMinimum(0);
    spinBox->setSingleStep(1);
    spinBox->setValue(0);
    spinBox->setEnabled(false);
    return spinBox;
  }
}
