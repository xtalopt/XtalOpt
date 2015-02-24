/********************************************************************************
** Form generated from reading UI file 'tab_conformers.ui'
**
** Created by: Qt User Interface Compiler version 4.8.6
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TAB_CONFORMERS_H
#define UI_TAB_CONFORMERS_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QGridLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QSpinBox>
#include <QtGui/QTableWidget>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Tab_Conformers
{
public:
    QGridLayout *gridLayout;
    QLabel *label;
    QComboBox *combo_mol;
    QTableWidget *table_conformers;
    QLabel *label_2;
    QComboBox *combo_opt;
    QLabel *label_3;
    QSpinBox *spin_nConformers;
    QCheckBox *cb_allConformers;
    QPushButton *push_generate;

    void setupUi(QWidget *Tab_Conformers)
    {
        if (Tab_Conformers->objectName().isEmpty())
            Tab_Conformers->setObjectName(QString::fromUtf8("Tab_Conformers"));
        Tab_Conformers->resize(608, 308);
        gridLayout = new QGridLayout(Tab_Conformers);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        label = new QLabel(Tab_Conformers);
        label->setObjectName(QString::fromUtf8("label"));

        gridLayout->addWidget(label, 0, 0, 1, 1);

        combo_mol = new QComboBox(Tab_Conformers);
        combo_mol->setObjectName(QString::fromUtf8("combo_mol"));
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(combo_mol->sizePolicy().hasHeightForWidth());
        combo_mol->setSizePolicy(sizePolicy);
        combo_mol->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);
        combo_mol->setMinimumContentsLength(10);

        gridLayout->addWidget(combo_mol, 0, 1, 1, 1);

        table_conformers = new QTableWidget(Tab_Conformers);
        if (table_conformers->columnCount() < 3)
            table_conformers->setColumnCount(3);
        QTableWidgetItem *__qtablewidgetitem = new QTableWidgetItem();
        table_conformers->setHorizontalHeaderItem(0, __qtablewidgetitem);
        QTableWidgetItem *__qtablewidgetitem1 = new QTableWidgetItem();
        table_conformers->setHorizontalHeaderItem(1, __qtablewidgetitem1);
        QTableWidgetItem *__qtablewidgetitem2 = new QTableWidgetItem();
        table_conformers->setHorizontalHeaderItem(2, __qtablewidgetitem2);
        table_conformers->setObjectName(QString::fromUtf8("table_conformers"));
        table_conformers->setSelectionMode(QAbstractItemView::SingleSelection);
        table_conformers->setSelectionBehavior(QAbstractItemView::SelectRows);

        gridLayout->addWidget(table_conformers, 2, 0, 1, 9);

        label_2 = new QLabel(Tab_Conformers);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        gridLayout->addWidget(label_2, 1, 0, 1, 1);

        combo_opt = new QComboBox(Tab_Conformers);
        combo_opt->setObjectName(QString::fromUtf8("combo_opt"));

        gridLayout->addWidget(combo_opt, 1, 1, 1, 1);

        label_3 = new QLabel(Tab_Conformers);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        gridLayout->addWidget(label_3, 1, 2, 1, 1);

        spin_nConformers = new QSpinBox(Tab_Conformers);
        spin_nConformers->setObjectName(QString::fromUtf8("spin_nConformers"));
        spin_nConformers->setMinimum(1);
        spin_nConformers->setMaximum(999999);
        spin_nConformers->setValue(100);

        gridLayout->addWidget(spin_nConformers, 1, 3, 1, 1);

        cb_allConformers = new QCheckBox(Tab_Conformers);
        cb_allConformers->setObjectName(QString::fromUtf8("cb_allConformers"));

        gridLayout->addWidget(cb_allConformers, 1, 4, 1, 1);

        push_generate = new QPushButton(Tab_Conformers);
        push_generate->setObjectName(QString::fromUtf8("push_generate"));

        gridLayout->addWidget(push_generate, 0, 2, 1, 1);

#ifndef QT_NO_SHORTCUT
        label->setBuddy(combo_mol);
        label_2->setBuddy(combo_opt);
        label_3->setBuddy(spin_nConformers);
#endif // QT_NO_SHORTCUT
        QWidget::setTabOrder(combo_mol, push_generate);
        QWidget::setTabOrder(push_generate, combo_opt);
        QWidget::setTabOrder(combo_opt, spin_nConformers);
        QWidget::setTabOrder(spin_nConformers, cb_allConformers);
        QWidget::setTabOrder(cb_allConformers, table_conformers);

        retranslateUi(Tab_Conformers);
        QObject::connect(cb_allConformers, SIGNAL(toggled(bool)), spin_nConformers, SLOT(setDisabled(bool)));

        combo_opt->setCurrentIndex(2);


        QMetaObject::connectSlotsByName(Tab_Conformers);
    } // setupUi

    void retranslateUi(QWidget *Tab_Conformers)
    {
        Tab_Conformers->setWindowTitle(QApplication::translate("Tab_Conformers", "Form", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("Tab_Conformers", "&Molecule:", 0, QApplication::UnicodeUTF8));
        QTableWidgetItem *___qtablewidgetitem = table_conformers->horizontalHeaderItem(0);
        ___qtablewidgetitem->setText(QApplication::translate("Tab_Conformers", "Conformer", 0, QApplication::UnicodeUTF8));
        QTableWidgetItem *___qtablewidgetitem1 = table_conformers->horizontalHeaderItem(1);
        ___qtablewidgetitem1->setText(QApplication::translate("Tab_Conformers", "Energy", 0, QApplication::UnicodeUTF8));
        QTableWidgetItem *___qtablewidgetitem2 = table_conformers->horizontalHeaderItem(2);
        ___qtablewidgetitem2->setText(QApplication::translate("Tab_Conformers", "Probability", 0, QApplication::UnicodeUTF8));
        label_2->setText(QApplication::translate("Tab_Conformers", "Method:", 0, QApplication::UnicodeUTF8));
        combo_opt->clear();
        combo_opt->insertItems(0, QStringList()
         << QApplication::translate("Tab_Conformers", "Gaussian 03", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Tab_Conformers", "Ghemical", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Tab_Conformers", "MMFF94", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Tab_Conformers", "MMFF94s", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Tab_Conformers", "UFF", 0, QApplication::UnicodeUTF8)
        );
        label_3->setText(QApplication::translate("Tab_Conformers", "&Number of conformers:", 0, QApplication::UnicodeUTF8));
        cb_allConformers->setText(QApplication::translate("Tab_Conformers", "&All", 0, QApplication::UnicodeUTF8));
        push_generate->setText(QApplication::translate("Tab_Conformers", "&Generate conformers", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class Tab_Conformers: public Ui_Tab_Conformers {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TAB_CONFORMERS_H
