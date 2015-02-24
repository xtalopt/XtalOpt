/********************************************************************************
** Form generated from reading UI file 'tab_init.ui'
**
** Created by: Qt User Interface Compiler version 4.8.6
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TAB_INIT_H
#define UI_TAB_INIT_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QDoubleSpinBox>
#include <QtGui/QFrame>
#include <QtGui/QGridLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QTableWidget>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Tab_Init
{
public:
    QGridLayout *gridLayout;
    QGroupBox *groupBox_2;
    QGridLayout *gridLayout_4;
    QLabel *label_14;
    QLineEdit *edit_composition;
    QTableWidget *table_comp;
    QGroupBox *groupBox;
    QDoubleSpinBox *spin_b_max;
    QDoubleSpinBox *spin_gamma_min;
    QLabel *label_11;
    QLabel *label_7;
    QDoubleSpinBox *spin_beta_min;
    QDoubleSpinBox *spin_c_max;
    QDoubleSpinBox *spin_alpha_max;
    QLabel *label_6;
    QLabel *label_8;
    QDoubleSpinBox *spin_b_min;
    QLabel *label_3;
    QLabel *label_5;
    QLabel *label_2;
    QLabel *label_4;
    QDoubleSpinBox *spin_a_min;
    QDoubleSpinBox *spin_beta_max;
    QDoubleSpinBox *spin_vol_min;
    QDoubleSpinBox *spin_vol_max;
    QFrame *line;
    QDoubleSpinBox *spin_fixedVolume;
    QDoubleSpinBox *spin_c_min;
    QDoubleSpinBox *spin_alpha_min;
    QDoubleSpinBox *spin_gamma_max;
    QCheckBox *cb_mitosis;
    QDoubleSpinBox *spin_a_max;
    QDoubleSpinBox *spin_scaleFactor;
    QComboBox *combo_divisions;
    QDoubleSpinBox *spin_minRadius;
    QLabel *label_12;
    QCheckBox *cb_interatomicDistanceLimit;
    QCheckBox *cb_fixedVolume;
    QLabel *label;
    QLabel *label_9;
    QFrame *line_2;
    QFrame *line_3;
    QComboBox *combo_a;
    QComboBox *combo_b;
    QComboBox *combo_c;
    QLabel *label_15;
    QLabel *label_16;
    QLabel *label_17;
    QLabel *label_13;
    QLabel *label_18;

    void setupUi(QWidget *Tab_Init)
    {
        if (Tab_Init->objectName().isEmpty())
            Tab_Init->setObjectName(QString::fromUtf8("Tab_Init"));
        Tab_Init->resize(1040, 686);
        gridLayout = new QGridLayout(Tab_Init);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        groupBox_2 = new QGroupBox(Tab_Init);
        groupBox_2->setObjectName(QString::fromUtf8("groupBox_2"));
        gridLayout_4 = new QGridLayout(groupBox_2);
        gridLayout_4->setObjectName(QString::fromUtf8("gridLayout_4"));
        label_14 = new QLabel(groupBox_2);
        label_14->setObjectName(QString::fromUtf8("label_14"));
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(label_14->sizePolicy().hasHeightForWidth());
        label_14->setSizePolicy(sizePolicy);

        gridLayout_4->addWidget(label_14, 0, 1, 1, 1);

        edit_composition = new QLineEdit(groupBox_2);
        edit_composition->setObjectName(QString::fromUtf8("edit_composition"));

        gridLayout_4->addWidget(edit_composition, 0, 2, 1, 2);

        table_comp = new QTableWidget(groupBox_2);
        if (table_comp->columnCount() < 5)
            table_comp->setColumnCount(5);
        QTableWidgetItem *__qtablewidgetitem = new QTableWidgetItem();
        table_comp->setHorizontalHeaderItem(0, __qtablewidgetitem);
        QTableWidgetItem *__qtablewidgetitem1 = new QTableWidgetItem();
        table_comp->setHorizontalHeaderItem(1, __qtablewidgetitem1);
        QTableWidgetItem *__qtablewidgetitem2 = new QTableWidgetItem();
        table_comp->setHorizontalHeaderItem(2, __qtablewidgetitem2);
        QTableWidgetItem *__qtablewidgetitem3 = new QTableWidgetItem();
        table_comp->setHorizontalHeaderItem(3, __qtablewidgetitem3);
        QTableWidgetItem *__qtablewidgetitem4 = new QTableWidgetItem();
        table_comp->setHorizontalHeaderItem(4, __qtablewidgetitem4);
        table_comp->setObjectName(QString::fromUtf8("table_comp"));
        table_comp->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table_comp->setSortingEnabled(true);
        table_comp->horizontalHeader()->setMinimumSectionSize(9);
        table_comp->horizontalHeader()->setStretchLastSection(true);

        gridLayout_4->addWidget(table_comp, 1, 1, 1, 3);


        gridLayout->addWidget(groupBox_2, 0, 0, 1, 1);

        groupBox = new QGroupBox(Tab_Init);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        spin_b_max = new QDoubleSpinBox(groupBox);
        spin_b_max->setObjectName(QString::fromUtf8("spin_b_max"));
        spin_b_max->setGeometry(QRect(320, 90, 171, 24));
        spin_b_max->setDecimals(5);
        spin_b_max->setMaximum(9999.99);
        spin_b_max->setSingleStep(0.1);
        spin_b_max->setValue(10);
        spin_gamma_min = new QDoubleSpinBox(groupBox);
        spin_gamma_min->setObjectName(QString::fromUtf8("spin_gamma_min"));
        spin_gamma_min->setGeometry(QRect(121, 226, 171, 24));
        spin_gamma_min->setDecimals(5);
        spin_gamma_min->setMaximum(180);
        spin_gamma_min->setSingleStep(0.1);
        spin_gamma_min->setValue(60);
        label_11 = new QLabel(groupBox);
        label_11->setObjectName(QString::fromUtf8("label_11"));
        label_11->setGeometry(QRect(30, 226, 71, 20));
        label_7 = new QLabel(groupBox);
        label_7->setObjectName(QString::fromUtf8("label_7"));
        label_7->setGeometry(QRect(320, 31, 71, 16));
        label_7->setAlignment(Qt::AlignCenter);
        spin_beta_min = new QDoubleSpinBox(groupBox);
        spin_beta_min->setObjectName(QString::fromUtf8("spin_beta_min"));
        spin_beta_min->setGeometry(QRect(121, 192, 171, 24));
        spin_beta_min->setDecimals(5);
        spin_beta_min->setMaximum(180);
        spin_beta_min->setSingleStep(0.1);
        spin_beta_min->setValue(60);
        spin_c_max = new QDoubleSpinBox(groupBox);
        spin_c_max->setObjectName(QString::fromUtf8("spin_c_max"));
        spin_c_max->setGeometry(QRect(320, 124, 171, 24));
        spin_c_max->setDecimals(5);
        spin_c_max->setMaximum(9999.99);
        spin_c_max->setSingleStep(0.1);
        spin_c_max->setValue(10);
        spin_alpha_max = new QDoubleSpinBox(groupBox);
        spin_alpha_max->setObjectName(QString::fromUtf8("spin_alpha_max"));
        spin_alpha_max->setGeometry(QRect(320, 158, 171, 24));
        spin_alpha_max->setDecimals(5);
        spin_alpha_max->setMaximum(180);
        spin_alpha_max->setSingleStep(0.1);
        spin_alpha_max->setValue(120);
        label_6 = new QLabel(groupBox);
        label_6->setObjectName(QString::fromUtf8("label_6"));
        label_6->setGeometry(QRect(121, 31, 61, 16));
        QSizePolicy sizePolicy1(QSizePolicy::Minimum, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(label_6->sizePolicy().hasHeightForWidth());
        label_6->setSizePolicy(sizePolicy1);
        label_6->setAlignment(Qt::AlignCenter);
        label_8 = new QLabel(groupBox);
        label_8->setObjectName(QString::fromUtf8("label_8"));
        label_8->setGeometry(QRect(30, 192, 71, 20));
        spin_b_min = new QDoubleSpinBox(groupBox);
        spin_b_min->setObjectName(QString::fromUtf8("spin_b_min"));
        spin_b_min->setGeometry(QRect(121, 90, 171, 24));
        spin_b_min->setDecimals(5);
        spin_b_min->setSingleStep(0.1);
        spin_b_min->setValue(3);
        label_3 = new QLabel(groupBox);
        label_3->setObjectName(QString::fromUtf8("label_3"));
        label_3->setGeometry(QRect(17, 90, 91, 20));
        label_5 = new QLabel(groupBox);
        label_5->setObjectName(QString::fromUtf8("label_5"));
        label_5->setGeometry(QRect(16, 56, 91, 20));
        label_2 = new QLabel(groupBox);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setGeometry(QRect(18, 124, 91, 20));
        label_4 = new QLabel(groupBox);
        label_4->setObjectName(QString::fromUtf8("label_4"));
        label_4->setGeometry(QRect(30, 158, 71, 20));
        spin_a_min = new QDoubleSpinBox(groupBox);
        spin_a_min->setObjectName(QString::fromUtf8("spin_a_min"));
        spin_a_min->setGeometry(QRect(121, 56, 171, 24));
        spin_a_min->setDecimals(5);
        spin_a_min->setSingleStep(0.1);
        spin_a_min->setValue(3);
        spin_beta_max = new QDoubleSpinBox(groupBox);
        spin_beta_max->setObjectName(QString::fromUtf8("spin_beta_max"));
        spin_beta_max->setGeometry(QRect(320, 192, 171, 24));
        spin_beta_max->setDecimals(5);
        spin_beta_max->setMaximum(180);
        spin_beta_max->setSingleStep(0.1);
        spin_beta_max->setValue(120);
        spin_vol_min = new QDoubleSpinBox(groupBox);
        spin_vol_min->setObjectName(QString::fromUtf8("spin_vol_min"));
        spin_vol_min->setGeometry(QRect(121, 270, 171, 24));
        spin_vol_min->setDecimals(2);
        spin_vol_min->setMaximum(1e+06);
        spin_vol_min->setSingleStep(1);
        spin_vol_min->setValue(0);
        spin_vol_max = new QDoubleSpinBox(groupBox);
        spin_vol_max->setObjectName(QString::fromUtf8("spin_vol_max"));
        spin_vol_max->setGeometry(QRect(320, 270, 171, 24));
        spin_vol_max->setDecimals(2);
        spin_vol_max->setMaximum(1e+06);
        spin_vol_max->setSingleStep(1);
        spin_vol_max->setValue(1e+06);
        line = new QFrame(groupBox);
        line->setObjectName(QString::fromUtf8("line"));
        line->setGeometry(QRect(297, 56, 16, 238));
        line->setFrameShape(QFrame::VLine);
        line->setFrameShadow(QFrame::Sunken);
        spin_fixedVolume = new QDoubleSpinBox(groupBox);
        spin_fixedVolume->setObjectName(QString::fromUtf8("spin_fixedVolume"));
        spin_fixedVolume->setEnabled(false);
        spin_fixedVolume->setGeometry(QRect(320, 321, 171, 24));
        spin_fixedVolume->setDecimals(5);
        spin_fixedVolume->setMaximum(999999);
        spin_fixedVolume->setSingleStep(0.1);
        spin_fixedVolume->setValue(500);
        spin_c_min = new QDoubleSpinBox(groupBox);
        spin_c_min->setObjectName(QString::fromUtf8("spin_c_min"));
        spin_c_min->setGeometry(QRect(121, 124, 171, 24));
        spin_c_min->setDecimals(5);
        spin_c_min->setSingleStep(0.1);
        spin_c_min->setValue(3);
        spin_alpha_min = new QDoubleSpinBox(groupBox);
        spin_alpha_min->setObjectName(QString::fromUtf8("spin_alpha_min"));
        spin_alpha_min->setGeometry(QRect(121, 158, 171, 24));
        spin_alpha_min->setDecimals(5);
        spin_alpha_min->setMaximum(180);
        spin_alpha_min->setSingleStep(0.1);
        spin_alpha_min->setValue(60);
        spin_gamma_max = new QDoubleSpinBox(groupBox);
        spin_gamma_max->setObjectName(QString::fromUtf8("spin_gamma_max"));
        spin_gamma_max->setGeometry(QRect(320, 226, 171, 24));
        spin_gamma_max->setDecimals(5);
        spin_gamma_max->setMaximum(180);
        spin_gamma_max->setSingleStep(0.1);
        spin_gamma_max->setValue(120);
        cb_mitosis = new QCheckBox(groupBox);
        cb_mitosis->setObjectName(QString::fromUtf8("cb_mitosis"));
        cb_mitosis->setGeometry(QRect(70, 365, 81, 18));
        cb_mitosis->setChecked(false);
        spin_a_max = new QDoubleSpinBox(groupBox);
        spin_a_max->setObjectName(QString::fromUtf8("spin_a_max"));
        spin_a_max->setGeometry(QRect(320, 56, 171, 24));
        spin_a_max->setDecimals(5);
        spin_a_max->setMaximum(9999.99);
        spin_a_max->setSingleStep(0.1);
        spin_a_max->setValue(10);
        spin_scaleFactor = new QDoubleSpinBox(groupBox);
        spin_scaleFactor->setObjectName(QString::fromUtf8("spin_scaleFactor"));
        spin_scaleFactor->setEnabled(false);
        spin_scaleFactor->setGeometry(QRect(390, 431, 101, 24));
        spin_scaleFactor->setDecimals(2);
        spin_scaleFactor->setMaximum(1);
        spin_scaleFactor->setSingleStep(0.05);
        spin_scaleFactor->setValue(0.5);
        combo_divisions = new QComboBox(groupBox);
        combo_divisions->setObjectName(QString::fromUtf8("combo_divisions"));
        combo_divisions->setEnabled(false);
        combo_divisions->setGeometry(QRect(390, 360, 91, 26));
        spin_minRadius = new QDoubleSpinBox(groupBox);
        spin_minRadius->setObjectName(QString::fromUtf8("spin_minRadius"));
        spin_minRadius->setEnabled(false);
        spin_minRadius->setGeometry(QRect(390, 457, 101, 24));
        spin_minRadius->setDecimals(2);
        spin_minRadius->setSingleStep(0.05);
        spin_minRadius->setValue(0.25);
        label_12 = new QLabel(groupBox);
        label_12->setObjectName(QString::fromUtf8("label_12"));
        label_12->setEnabled(false);
        label_12->setGeometry(QRect(280, 363, 101, 21));
        cb_interatomicDistanceLimit = new QCheckBox(groupBox);
        cb_interatomicDistanceLimit->setObjectName(QString::fromUtf8("cb_interatomicDistanceLimit"));
        cb_interatomicDistanceLimit->setGeometry(QRect(70, 436, 201, 18));
        cb_fixedVolume = new QCheckBox(groupBox);
        cb_fixedVolume->setObjectName(QString::fromUtf8("cb_fixedVolume"));
        cb_fixedVolume->setGeometry(QRect(70, 324, 121, 18));
        label = new QLabel(groupBox);
        label->setObjectName(QString::fromUtf8("label"));
        label->setEnabled(false);
        label->setGeometry(QRect(300, 435, 91, 20));
        label_9 = new QLabel(groupBox);
        label_9->setObjectName(QString::fromUtf8("label_9"));
        label_9->setEnabled(false);
        label_9->setGeometry(QRect(270, 460, 111, 20));
        line_2 = new QFrame(groupBox);
        line_2->setObjectName(QString::fromUtf8("line_2"));
        line_2->setGeometry(QRect(130, 342, 301, 20));
        line_2->setFrameShape(QFrame::HLine);
        line_2->setFrameShadow(QFrame::Sunken);
        line_3 = new QFrame(groupBox);
        line_3->setObjectName(QString::fromUtf8("line_3"));
        line_3->setGeometry(QRect(130, 412, 301, 20));
        line_3->setFrameShape(QFrame::HLine);
        line_3->setFrameShadow(QFrame::Sunken);
        combo_a = new QComboBox(groupBox);
        combo_a->setObjectName(QString::fromUtf8("combo_a"));
        combo_a->setEnabled(false);
        combo_a->setGeometry(QRect(220, 390, 61, 26));
        combo_b = new QComboBox(groupBox);
        combo_b->setObjectName(QString::fromUtf8("combo_b"));
        combo_b->setEnabled(false);
        combo_b->setGeometry(QRect(320, 390, 61, 26));
        combo_c = new QComboBox(groupBox);
        combo_c->setObjectName(QString::fromUtf8("combo_c"));
        combo_c->setEnabled(false);
        combo_c->setGeometry(QRect(420, 390, 61, 26));
        label_15 = new QLabel(groupBox);
        label_15->setObjectName(QString::fromUtf8("label_15"));
        label_15->setEnabled(false);
        label_15->setGeometry(QRect(200, 393, 16, 20));
        label_16 = new QLabel(groupBox);
        label_16->setObjectName(QString::fromUtf8("label_16"));
        label_16->setEnabled(false);
        label_16->setGeometry(QRect(300, 393, 16, 20));
        label_17 = new QLabel(groupBox);
        label_17->setObjectName(QString::fromUtf8("label_17"));
        label_17->setEnabled(false);
        label_17->setGeometry(QRect(400, 393, 16, 20));
        label_13 = new QLabel(groupBox);
        label_13->setObjectName(QString::fromUtf8("label_13"));
        label_13->setGeometry(QRect(220, 324, 81, 20));
        label_18 = new QLabel(groupBox);
        label_18->setObjectName(QString::fromUtf8("label_18"));
        label_18->setGeometry(QRect(20, 274, 81, 20));

        gridLayout->addWidget(groupBox, 0, 1, 1, 1);

#ifndef QT_NO_SHORTCUT
        label_14->setBuddy(edit_composition);
        label_11->setBuddy(spin_gamma_min);
        label_8->setBuddy(spin_beta_min);
        label_3->setBuddy(spin_b_min);
        label_5->setBuddy(spin_a_min);
        label_2->setBuddy(spin_c_min);
        label_4->setBuddy(spin_alpha_min);
        label_12->setBuddy(combo_divisions);
        label->setBuddy(spin_scaleFactor);
        label_9->setBuddy(spin_minRadius);
        label_13->setBuddy(spin_c_min);
        label_18->setBuddy(spin_c_min);
#endif // QT_NO_SHORTCUT
        QWidget::setTabOrder(edit_composition, spin_a_min);
        QWidget::setTabOrder(spin_a_min, spin_a_max);
        QWidget::setTabOrder(spin_a_max, spin_b_min);
        QWidget::setTabOrder(spin_b_min, spin_b_max);
        QWidget::setTabOrder(spin_b_max, spin_c_min);
        QWidget::setTabOrder(spin_c_min, spin_c_max);
        QWidget::setTabOrder(spin_c_max, spin_alpha_min);
        QWidget::setTabOrder(spin_alpha_min, spin_alpha_max);
        QWidget::setTabOrder(spin_alpha_max, spin_beta_min);
        QWidget::setTabOrder(spin_beta_min, spin_beta_max);
        QWidget::setTabOrder(spin_beta_max, spin_gamma_min);
        QWidget::setTabOrder(spin_gamma_min, spin_gamma_max);
        QWidget::setTabOrder(spin_gamma_max, spin_vol_min);
        QWidget::setTabOrder(spin_vol_min, spin_vol_max);
        QWidget::setTabOrder(spin_vol_max, spin_fixedVolume);
        QWidget::setTabOrder(spin_fixedVolume, spin_scaleFactor);
        QWidget::setTabOrder(spin_scaleFactor, spin_minRadius);
        QWidget::setTabOrder(spin_minRadius, table_comp);

        retranslateUi(Tab_Init);
        QObject::connect(cb_interatomicDistanceLimit, SIGNAL(toggled(bool)), spin_scaleFactor, SLOT(setEnabled(bool)));
        QObject::connect(cb_fixedVolume, SIGNAL(toggled(bool)), spin_fixedVolume, SLOT(setEnabled(bool)));
        QObject::connect(cb_fixedVolume, SIGNAL(toggled(bool)), spin_vol_min, SLOT(setDisabled(bool)));
        QObject::connect(cb_fixedVolume, SIGNAL(toggled(bool)), spin_vol_max, SLOT(setDisabled(bool)));
        QObject::connect(cb_interatomicDistanceLimit, SIGNAL(toggled(bool)), label, SLOT(setEnabled(bool)));
        QObject::connect(cb_interatomicDistanceLimit, SIGNAL(toggled(bool)), label_9, SLOT(setEnabled(bool)));
        QObject::connect(cb_interatomicDistanceLimit, SIGNAL(toggled(bool)), spin_minRadius, SLOT(setEnabled(bool)));
        QObject::connect(cb_mitosis, SIGNAL(toggled(bool)), label_12, SLOT(setEnabled(bool)));
        QObject::connect(cb_mitosis, SIGNAL(toggled(bool)), combo_divisions, SLOT(setEnabled(bool)));
        QObject::connect(cb_mitosis, SIGNAL(toggled(bool)), label_15, SLOT(setEnabled(bool)));
        QObject::connect(cb_mitosis, SIGNAL(toggled(bool)), label_16, SLOT(setEnabled(bool)));
        QObject::connect(cb_mitosis, SIGNAL(toggled(bool)), label_17, SLOT(setEnabled(bool)));
        QObject::connect(cb_mitosis, SIGNAL(toggled(bool)), combo_a, SLOT(setEnabled(bool)));
        QObject::connect(cb_mitosis, SIGNAL(toggled(bool)), combo_b, SLOT(setEnabled(bool)));
        QObject::connect(cb_mitosis, SIGNAL(toggled(bool)), combo_c, SLOT(setEnabled(bool)));

        QMetaObject::connectSlotsByName(Tab_Init);
    } // setupUi

    void retranslateUi(QWidget *Tab_Init)
    {
        Tab_Init->setWindowTitle(QApplication::translate("Tab_Init", "Form", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        Tab_Init->setToolTip(QString());
#endif // QT_NO_TOOLTIP
        groupBox_2->setTitle(QApplication::translate("Tab_Init", "Composition", 0, QApplication::UnicodeUTF8));
        label_14->setText(QApplication::translate("Tab_Init", "&Cell composition:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        edit_composition->setToolTip(QApplication::translate("Tab_Init", "Type the composition of the crystal here,\n"
"\n"
"Examples:\n"
"ti1o2\n"
"Ti1O2\n"
"TI1O2\n"
"Ti1 O2\n"
"Ti 1 o 2\n"
"etc.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        QTableWidgetItem *___qtablewidgetitem = table_comp->horizontalHeaderItem(0);
        ___qtablewidgetitem->setText(QApplication::translate("Tab_Init", "Symbol", 0, QApplication::UnicodeUTF8));
        QTableWidgetItem *___qtablewidgetitem1 = table_comp->horizontalHeaderItem(1);
        ___qtablewidgetitem1->setText(QApplication::translate("Tab_Init", "Z", 0, QApplication::UnicodeUTF8));
        QTableWidgetItem *___qtablewidgetitem2 = table_comp->horizontalHeaderItem(2);
        ___qtablewidgetitem2->setText(QApplication::translate("Tab_Init", "#", 0, QApplication::UnicodeUTF8));
        QTableWidgetItem *___qtablewidgetitem3 = table_comp->horizontalHeaderItem(3);
        ___qtablewidgetitem3->setText(QApplication::translate("Tab_Init", "Mass", 0, QApplication::UnicodeUTF8));
        QTableWidgetItem *___qtablewidgetitem4 = table_comp->horizontalHeaderItem(4);
        ___qtablewidgetitem4->setText(QApplication::translate("Tab_Init", "Min. Radius", 0, QApplication::UnicodeUTF8));
        groupBox->setTitle(QApplication::translate("Tab_Init", "Unit Cell Parameters", 0, QApplication::UnicodeUTF8));
        label_11->setText(QApplication::translate("Tab_Init", "Angle \316\263 (\302\260):", 0, QApplication::UnicodeUTF8));
        label_7->setText(QApplication::translate("Tab_Init", "Maximum", 0, QApplication::UnicodeUTF8));
        label_6->setText(QApplication::translate("Tab_Init", "Minimum", 0, QApplication::UnicodeUTF8));
        label_8->setText(QApplication::translate("Tab_Init", "Angle \316\262 (\302\260):", 0, QApplication::UnicodeUTF8));
        label_3->setText(QApplication::translate("Tab_Init", "Length B (\303\205):", 0, QApplication::UnicodeUTF8));
        label_5->setText(QApplication::translate("Tab_Init", "Length A (\303\205):", 0, QApplication::UnicodeUTF8));
        label_2->setText(QApplication::translate("Tab_Init", "Length C (\303\205):", 0, QApplication::UnicodeUTF8));
        label_4->setText(QApplication::translate("Tab_Init", "Angle \316\261 (\302\260):", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        spin_fixedVolume->setToolTip(QApplication::translate("Tab_Init", "Use this to specify a starting volume for the structure before optimizing.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        cb_mitosis->setText(QApplication::translate("Tab_Init", "Mitosis", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        spin_scaleFactor->setToolTip(QApplication::translate("Tab_Init", "Use this to limit the shortest interatomic distance between\n"
"atoms. This ensures that atoms aren't overlapping.\n"
"\n"
"The value specified here is multiplied by each atom's covalent \n"
"radius, and the sum of the scaled radii are used to enforce a\n"
"minimum atomic separation between atoms.\n"
"\n"
"The default recommended value is 0.5. This ensures that atoms will not be unphysically close (which may prevent\n"
"certain optimization codes from working). Using a higher value\n"
"is acceptable while creating random structures in the first \n"
"generation, but may prevent the crossover operator from being\n"
"able to create offspring that meet this restriction.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        spin_scaleFactor->setSuffix(QApplication::translate("Tab_Init", " * radii", 0, QApplication::UnicodeUTF8));
        spin_minRadius->setSuffix(QApplication::translate("Tab_Init", " \303\205", 0, QApplication::UnicodeUTF8));
        label_12->setText(QApplication::translate("Tab_Init", "# of Divisions:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        cb_interatomicDistanceLimit->setToolTip(QApplication::translate("Tab_Init", "Use this to limit the shortest interatomic distance between atoms. This ensures that atoms aren't overlapping.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        cb_interatomicDistanceLimit->setText(QApplication::translate("Tab_Init", "Limit interatomic distance", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        cb_fixedVolume->setToolTip(QApplication::translate("Tab_Init", "Use this to specify a starting volume for the structure before optimizing.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        cb_fixedVolume->setText(QApplication::translate("Tab_Init", "Fixed volume", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("Tab_Init", "Scale factor:", 0, QApplication::UnicodeUTF8));
        label_9->setText(QApplication::translate("Tab_Init", "Minimum radius:", 0, QApplication::UnicodeUTF8));
        label_15->setText(QApplication::translate("Tab_Init", "a:", 0, QApplication::UnicodeUTF8));
        label_16->setText(QApplication::translate("Tab_Init", "b:", 0, QApplication::UnicodeUTF8));
        label_17->setText(QApplication::translate("Tab_Init", "c:", 0, QApplication::UnicodeUTF8));
        label_13->setText(QApplication::translate("Tab_Init", "<html><head/><body><p>Volume (\303\205<span style=\" vertical-align:super;\">3</span>):</p></body></html>", 0, QApplication::UnicodeUTF8));
        label_18->setText(QApplication::translate("Tab_Init", "<html><head/><body><p>Volume (\303\205<span style=\" vertical-align:super;\">3</span>):</p></body></html>", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class Tab_Init: public Ui_Tab_Init {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TAB_INIT_H
