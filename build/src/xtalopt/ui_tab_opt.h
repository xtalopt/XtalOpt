/********************************************************************************
** Form generated from reading UI file 'tab_opt.ui'
**
** Created by: Qt User Interface Compiler version 4.8.6
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TAB_OPT_H
#define UI_TAB_OPT_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QDoubleSpinBox>
#include <QtGui/QGridLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QListWidget>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QSpinBox>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Tab_Opt
{
public:
    QGridLayout *gridLayout;
    QGroupBox *groupBox_6;
    QGridLayout *gridLayout_7;
    QLabel *label_29;
    QLabel *label_27;
    QSpinBox *spin_p_strip;
    QDoubleSpinBox *spin_strip_strainStdev_min;
    QLabel *label_6;
    QDoubleSpinBox *spin_strip_amp_min;
    QLabel *label_8;
    QSpinBox *spin_strip_per1;
    QSpinBox *spin_strip_per2;
    QLabel *label_7;
    QSpacerItem *verticalSpacer_4;
    QDoubleSpinBox *spin_strip_strainStdev_max;
    QDoubleSpinBox *spin_strip_amp_max;
    QGroupBox *groupBox_7;
    QGridLayout *gridLayout_8;
    QLabel *label_31;
    QLabel *label_30;
    QSpinBox *spin_p_perm;
    QSpinBox *spin_perm_ex;
    QLabel *label_5;
    QDoubleSpinBox *spin_perm_strainStdev_max;
    QSpacerItem *verticalSpacer_5;
    QGroupBox *groupBox_4;
    QGridLayout *gridLayout_5;
    QLabel *label_26;
    QSpinBox *spin_popSize;
    QLabel *label_genTotal;
    QSpinBox *spin_contStructs;
    QCheckBox *cb_limitRunningJobs;
    QSpacerItem *verticalSpacer;
    QSpinBox *spin_runningJobLimit;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label_4;
    QSpinBox *spin_failLimit;
    QComboBox *combo_failAction;
    QSpinBox *spin_cutoff;
    QLabel *label_11;
    QGroupBox *groupBox_5;
    QGridLayout *gridLayout_6;
    QLabel *label_25;
    QSpinBox *spin_p_cross;
    QLabel *label_3;
    QSpinBox *spin_cross_minimumContribution;
    QSpacerItem *verticalSpacer_3;
    QGroupBox *groupBox;
    QGridLayout *gridLayout_2;
    QSpacerItem *verticalSpacer_2;
    QGroupBox *groupBox_8;
    QGridLayout *gridLayout_9;
    QLabel *label_10;
    QDoubleSpinBox *spin_tol_spg;
    QPushButton *push_spg_reset;
    QGroupBox *groupBox_9;
    QGridLayout *gridLayout_10;
    QLabel *label;
    QDoubleSpinBox *spin_tol_xcLength;
    QLabel *label_2;
    QDoubleSpinBox *spin_tol_xcAngle;
    QPushButton *push_dup_reset;
    QGroupBox *groupBox_3;
    QGridLayout *gridLayout_4;
    QGroupBox *groupBox_2;
    QGridLayout *gridLayout_3;
    QListWidget *list_seeds;
    QPushButton *push_addSeed;
    QPushButton *push_removeSeed;
    QSpacerItem *horizontalSpacer;
    QLabel *label_9;
    QSpinBox *spin_numInitial;

    void setupUi(QWidget *Tab_Opt)
    {
        if (Tab_Opt->objectName().isEmpty())
            Tab_Opt->setObjectName(QString::fromUtf8("Tab_Opt"));
        Tab_Opt->resize(1046, 627);
        gridLayout = new QGridLayout(Tab_Opt);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        groupBox_6 = new QGroupBox(Tab_Opt);
        groupBox_6->setObjectName(QString::fromUtf8("groupBox_6"));
        gridLayout_7 = new QGridLayout(groupBox_6);
        gridLayout_7->setObjectName(QString::fromUtf8("gridLayout_7"));
        label_29 = new QLabel(groupBox_6);
        label_29->setObjectName(QString::fromUtf8("label_29"));

        gridLayout_7->addWidget(label_29, 0, 0, 1, 1);

        label_27 = new QLabel(groupBox_6);
        label_27->setObjectName(QString::fromUtf8("label_27"));

        gridLayout_7->addWidget(label_27, 1, 0, 1, 1);

        spin_p_strip = new QSpinBox(groupBox_6);
        spin_p_strip->setObjectName(QString::fromUtf8("spin_p_strip"));
        spin_p_strip->setMaximum(100);
        spin_p_strip->setValue(50);

        gridLayout_7->addWidget(spin_p_strip, 0, 1, 1, 2);

        spin_strip_strainStdev_min = new QDoubleSpinBox(groupBox_6);
        spin_strip_strainStdev_min->setObjectName(QString::fromUtf8("spin_strip_strainStdev_min"));
        spin_strip_strainStdev_min->setDecimals(3);
        spin_strip_strainStdev_min->setMaximum(2);
        spin_strip_strainStdev_min->setSingleStep(0.1);
        spin_strip_strainStdev_min->setValue(0.5);

        gridLayout_7->addWidget(spin_strip_strainStdev_min, 1, 1, 1, 1);

        label_6 = new QLabel(groupBox_6);
        label_6->setObjectName(QString::fromUtf8("label_6"));

        gridLayout_7->addWidget(label_6, 2, 0, 1, 1);

        spin_strip_amp_min = new QDoubleSpinBox(groupBox_6);
        spin_strip_amp_min->setObjectName(QString::fromUtf8("spin_strip_amp_min"));
        spin_strip_amp_min->setDecimals(3);
        spin_strip_amp_min->setMaximum(1);
        spin_strip_amp_min->setSingleStep(0.05);
        spin_strip_amp_min->setValue(0.5);

        gridLayout_7->addWidget(spin_strip_amp_min, 2, 1, 1, 1);

        label_8 = new QLabel(groupBox_6);
        label_8->setObjectName(QString::fromUtf8("label_8"));

        gridLayout_7->addWidget(label_8, 4, 0, 1, 1);

        spin_strip_per1 = new QSpinBox(groupBox_6);
        spin_strip_per1->setObjectName(QString::fromUtf8("spin_strip_per1"));
        spin_strip_per1->setValue(1);

        gridLayout_7->addWidget(spin_strip_per1, 3, 1, 1, 2);

        spin_strip_per2 = new QSpinBox(groupBox_6);
        spin_strip_per2->setObjectName(QString::fromUtf8("spin_strip_per2"));
        spin_strip_per2->setValue(1);

        gridLayout_7->addWidget(spin_strip_per2, 4, 1, 1, 2);

        label_7 = new QLabel(groupBox_6);
        label_7->setObjectName(QString::fromUtf8("label_7"));

        gridLayout_7->addWidget(label_7, 3, 0, 1, 1);

        verticalSpacer_4 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout_7->addItem(verticalSpacer_4, 5, 0, 1, 1);

        spin_strip_strainStdev_max = new QDoubleSpinBox(groupBox_6);
        spin_strip_strainStdev_max->setObjectName(QString::fromUtf8("spin_strip_strainStdev_max"));
        spin_strip_strainStdev_max->setDecimals(3);
        spin_strip_strainStdev_max->setMaximum(2);
        spin_strip_strainStdev_max->setSingleStep(0.1);
        spin_strip_strainStdev_max->setValue(0.5);

        gridLayout_7->addWidget(spin_strip_strainStdev_max, 1, 2, 1, 1);

        spin_strip_amp_max = new QDoubleSpinBox(groupBox_6);
        spin_strip_amp_max->setObjectName(QString::fromUtf8("spin_strip_amp_max"));
        spin_strip_amp_max->setDecimals(3);
        spin_strip_amp_max->setMaximum(1);
        spin_strip_amp_max->setSingleStep(0.05);
        spin_strip_amp_max->setValue(1);

        gridLayout_7->addWidget(spin_strip_amp_max, 2, 2, 1, 1);


        gridLayout->addWidget(groupBox_6, 1, 1, 1, 1);

        groupBox_7 = new QGroupBox(Tab_Opt);
        groupBox_7->setObjectName(QString::fromUtf8("groupBox_7"));
        gridLayout_8 = new QGridLayout(groupBox_7);
        gridLayout_8->setObjectName(QString::fromUtf8("gridLayout_8"));
        label_31 = new QLabel(groupBox_7);
        label_31->setObjectName(QString::fromUtf8("label_31"));

        gridLayout_8->addWidget(label_31, 0, 0, 1, 1);

        label_30 = new QLabel(groupBox_7);
        label_30->setObjectName(QString::fromUtf8("label_30"));

        gridLayout_8->addWidget(label_30, 2, 0, 1, 1);

        spin_p_perm = new QSpinBox(groupBox_7);
        spin_p_perm->setObjectName(QString::fromUtf8("spin_p_perm"));
        spin_p_perm->setMaximum(100);
        spin_p_perm->setValue(35);

        gridLayout_8->addWidget(spin_p_perm, 0, 1, 1, 2);

        spin_perm_ex = new QSpinBox(groupBox_7);
        spin_perm_ex->setObjectName(QString::fromUtf8("spin_perm_ex"));
        spin_perm_ex->setMaximum(100);
        spin_perm_ex->setValue(4);

        gridLayout_8->addWidget(spin_perm_ex, 2, 1, 1, 2);

        label_5 = new QLabel(groupBox_7);
        label_5->setObjectName(QString::fromUtf8("label_5"));

        gridLayout_8->addWidget(label_5, 1, 0, 1, 1);

        spin_perm_strainStdev_max = new QDoubleSpinBox(groupBox_7);
        spin_perm_strainStdev_max->setObjectName(QString::fromUtf8("spin_perm_strainStdev_max"));
        spin_perm_strainStdev_max->setDecimals(3);
        spin_perm_strainStdev_max->setValue(0.5);

        gridLayout_8->addWidget(spin_perm_strainStdev_max, 1, 1, 1, 2);

        verticalSpacer_5 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout_8->addItem(verticalSpacer_5, 3, 0, 1, 1);


        gridLayout->addWidget(groupBox_7, 1, 2, 1, 1);

        groupBox_4 = new QGroupBox(Tab_Opt);
        groupBox_4->setObjectName(QString::fromUtf8("groupBox_4"));
        gridLayout_5 = new QGridLayout(groupBox_4);
        gridLayout_5->setObjectName(QString::fromUtf8("gridLayout_5"));
        label_26 = new QLabel(groupBox_4);
        label_26->setObjectName(QString::fromUtf8("label_26"));

        gridLayout_5->addWidget(label_26, 0, 0, 1, 1);

        spin_popSize = new QSpinBox(groupBox_4);
        spin_popSize->setObjectName(QString::fromUtf8("spin_popSize"));
        spin_popSize->setMaximum(9999);
        spin_popSize->setValue(20);

        gridLayout_5->addWidget(spin_popSize, 0, 1, 1, 1);

        label_genTotal = new QLabel(groupBox_4);
        label_genTotal->setObjectName(QString::fromUtf8("label_genTotal"));

        gridLayout_5->addWidget(label_genTotal, 1, 0, 1, 1);

        spin_contStructs = new QSpinBox(groupBox_4);
        spin_contStructs->setObjectName(QString::fromUtf8("spin_contStructs"));

        gridLayout_5->addWidget(spin_contStructs, 1, 1, 1, 1);

        cb_limitRunningJobs = new QCheckBox(groupBox_4);
        cb_limitRunningJobs->setObjectName(QString::fromUtf8("cb_limitRunningJobs"));

        gridLayout_5->addWidget(cb_limitRunningJobs, 3, 0, 1, 1);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout_5->addItem(verticalSpacer, 6, 0, 1, 1);

        spin_runningJobLimit = new QSpinBox(groupBox_4);
        spin_runningJobLimit->setObjectName(QString::fromUtf8("spin_runningJobLimit"));
        spin_runningJobLimit->setEnabled(false);
        spin_runningJobLimit->setValue(1);

        gridLayout_5->addWidget(spin_runningJobLimit, 3, 1, 1, 1);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        label_4 = new QLabel(groupBox_4);
        label_4->setObjectName(QString::fromUtf8("label_4"));
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(label_4->sizePolicy().hasHeightForWidth());
        label_4->setSizePolicy(sizePolicy);

        horizontalLayout_2->addWidget(label_4);

        spin_failLimit = new QSpinBox(groupBox_4);
        spin_failLimit->setObjectName(QString::fromUtf8("spin_failLimit"));
        QSizePolicy sizePolicy1(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(spin_failLimit->sizePolicy().hasHeightForWidth());
        spin_failLimit->setSizePolicy(sizePolicy1);
        spin_failLimit->setMaximum(100);

        horizontalLayout_2->addWidget(spin_failLimit);

        combo_failAction = new QComboBox(groupBox_4);
        combo_failAction->setObjectName(QString::fromUtf8("combo_failAction"));

        horizontalLayout_2->addWidget(combo_failAction);


        gridLayout_5->addLayout(horizontalLayout_2, 4, 0, 1, 2);

        spin_cutoff = new QSpinBox(groupBox_4);
        spin_cutoff->setObjectName(QString::fromUtf8("spin_cutoff"));
        spin_cutoff->setMaximum(100000);

        gridLayout_5->addWidget(spin_cutoff, 5, 1, 1, 1);

        label_11 = new QLabel(groupBox_4);
        label_11->setObjectName(QString::fromUtf8("label_11"));

        gridLayout_5->addWidget(label_11, 5, 0, 1, 1);


        gridLayout->addWidget(groupBox_4, 0, 1, 1, 1);

        groupBox_5 = new QGroupBox(Tab_Opt);
        groupBox_5->setObjectName(QString::fromUtf8("groupBox_5"));
        gridLayout_6 = new QGridLayout(groupBox_5);
        gridLayout_6->setObjectName(QString::fromUtf8("gridLayout_6"));
        label_25 = new QLabel(groupBox_5);
        label_25->setObjectName(QString::fromUtf8("label_25"));

        gridLayout_6->addWidget(label_25, 2, 0, 1, 1);

        spin_p_cross = new QSpinBox(groupBox_5);
        spin_p_cross->setObjectName(QString::fromUtf8("spin_p_cross"));
        spin_p_cross->setMaximum(100);
        spin_p_cross->setValue(15);

        gridLayout_6->addWidget(spin_p_cross, 2, 1, 1, 1);

        label_3 = new QLabel(groupBox_5);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        gridLayout_6->addWidget(label_3, 3, 0, 1, 1);

        spin_cross_minimumContribution = new QSpinBox(groupBox_5);
        spin_cross_minimumContribution->setObjectName(QString::fromUtf8("spin_cross_minimumContribution"));
        spin_cross_minimumContribution->setMaximum(50);
        spin_cross_minimumContribution->setValue(20);

        gridLayout_6->addWidget(spin_cross_minimumContribution, 3, 1, 1, 1);

        verticalSpacer_3 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout_6->addItem(verticalSpacer_3, 4, 1, 1, 1);


        gridLayout->addWidget(groupBox_5, 1, 0, 1, 1);

        groupBox = new QGroupBox(Tab_Opt);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        gridLayout_2 = new QGridLayout(groupBox);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        verticalSpacer_2 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout_2->addItem(verticalSpacer_2, 6, 1, 1, 1);

        groupBox_8 = new QGroupBox(groupBox);
        groupBox_8->setObjectName(QString::fromUtf8("groupBox_8"));
        gridLayout_9 = new QGridLayout(groupBox_8);
        gridLayout_9->setObjectName(QString::fromUtf8("gridLayout_9"));
        label_10 = new QLabel(groupBox_8);
        label_10->setObjectName(QString::fromUtf8("label_10"));

        gridLayout_9->addWidget(label_10, 0, 0, 1, 1);

        spin_tol_spg = new QDoubleSpinBox(groupBox_8);
        spin_tol_spg->setObjectName(QString::fromUtf8("spin_tol_spg"));
        spin_tol_spg->setDecimals(3);
        spin_tol_spg->setSingleStep(0.005);

        gridLayout_9->addWidget(spin_tol_spg, 0, 1, 1, 1);

        push_spg_reset = new QPushButton(groupBox_8);
        push_spg_reset->setObjectName(QString::fromUtf8("push_spg_reset"));

        gridLayout_9->addWidget(push_spg_reset, 1, 0, 1, 2);


        gridLayout_2->addWidget(groupBox_8, 0, 1, 1, 2);

        groupBox_9 = new QGroupBox(groupBox);
        groupBox_9->setObjectName(QString::fromUtf8("groupBox_9"));
        gridLayout_10 = new QGridLayout(groupBox_9);
        gridLayout_10->setObjectName(QString::fromUtf8("gridLayout_10"));
        label = new QLabel(groupBox_9);
        label->setObjectName(QString::fromUtf8("label"));

        gridLayout_10->addWidget(label, 0, 0, 1, 1);

        spin_tol_xcLength = new QDoubleSpinBox(groupBox_9);
        spin_tol_xcLength->setObjectName(QString::fromUtf8("spin_tol_xcLength"));
        spin_tol_xcLength->setDecimals(3);
        spin_tol_xcLength->setSingleStep(0.005);

        gridLayout_10->addWidget(spin_tol_xcLength, 0, 1, 1, 1);

        label_2 = new QLabel(groupBox_9);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        gridLayout_10->addWidget(label_2, 1, 0, 1, 1);

        spin_tol_xcAngle = new QDoubleSpinBox(groupBox_9);
        spin_tol_xcAngle->setObjectName(QString::fromUtf8("spin_tol_xcAngle"));
        spin_tol_xcAngle->setDecimals(3);
        spin_tol_xcAngle->setMaximum(100);
        spin_tol_xcAngle->setSingleStep(0.05);

        gridLayout_10->addWidget(spin_tol_xcAngle, 1, 1, 1, 1);

        push_dup_reset = new QPushButton(groupBox_9);
        push_dup_reset->setObjectName(QString::fromUtf8("push_dup_reset"));

        gridLayout_10->addWidget(push_dup_reset, 2, 0, 1, 2);


        gridLayout_2->addWidget(groupBox_9, 1, 1, 1, 2);


        gridLayout->addWidget(groupBox, 0, 2, 1, 1);

        groupBox_3 = new QGroupBox(Tab_Opt);
        groupBox_3->setObjectName(QString::fromUtf8("groupBox_3"));
        QSizePolicy sizePolicy2(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(groupBox_3->sizePolicy().hasHeightForWidth());
        groupBox_3->setSizePolicy(sizePolicy2);
        gridLayout_4 = new QGridLayout(groupBox_3);
        gridLayout_4->setObjectName(QString::fromUtf8("gridLayout_4"));
        groupBox_2 = new QGroupBox(groupBox_3);
        groupBox_2->setObjectName(QString::fromUtf8("groupBox_2"));
        gridLayout_3 = new QGridLayout(groupBox_2);
        gridLayout_3->setObjectName(QString::fromUtf8("gridLayout_3"));
        list_seeds = new QListWidget(groupBox_2);
        list_seeds->setObjectName(QString::fromUtf8("list_seeds"));
        QSizePolicy sizePolicy3(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(list_seeds->sizePolicy().hasHeightForWidth());
        list_seeds->setSizePolicy(sizePolicy3);

        gridLayout_3->addWidget(list_seeds, 0, 0, 1, 3);

        push_addSeed = new QPushButton(groupBox_2);
        push_addSeed->setObjectName(QString::fromUtf8("push_addSeed"));
        sizePolicy1.setHeightForWidth(push_addSeed->sizePolicy().hasHeightForWidth());
        push_addSeed->setSizePolicy(sizePolicy1);

        gridLayout_3->addWidget(push_addSeed, 1, 0, 1, 1);

        push_removeSeed = new QPushButton(groupBox_2);
        push_removeSeed->setObjectName(QString::fromUtf8("push_removeSeed"));
        sizePolicy1.setHeightForWidth(push_removeSeed->sizePolicy().hasHeightForWidth());
        push_removeSeed->setSizePolicy(sizePolicy1);

        gridLayout_3->addWidget(push_removeSeed, 1, 2, 1, 1);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout_3->addItem(horizontalSpacer, 1, 1, 1, 1);


        gridLayout_4->addWidget(groupBox_2, 1, 0, 1, 2);

        label_9 = new QLabel(groupBox_3);
        label_9->setObjectName(QString::fromUtf8("label_9"));
        sizePolicy.setHeightForWidth(label_9->sizePolicy().hasHeightForWidth());
        label_9->setSizePolicy(sizePolicy);

        gridLayout_4->addWidget(label_9, 0, 0, 1, 1);

        spin_numInitial = new QSpinBox(groupBox_3);
        spin_numInitial->setObjectName(QString::fromUtf8("spin_numInitial"));
        QSizePolicy sizePolicy4(QSizePolicy::Maximum, QSizePolicy::Fixed);
        sizePolicy4.setHorizontalStretch(0);
        sizePolicy4.setVerticalStretch(0);
        sizePolicy4.setHeightForWidth(spin_numInitial->sizePolicy().hasHeightForWidth());
        spin_numInitial->setSizePolicy(sizePolicy4);
        spin_numInitial->setMaximum(9999);
        spin_numInitial->setValue(20);

        gridLayout_4->addWidget(spin_numInitial, 0, 1, 1, 1);


        gridLayout->addWidget(groupBox_3, 0, 0, 1, 1);

#ifndef QT_NO_SHORTCUT
        label_29->setBuddy(spin_p_strip);
        label_27->setBuddy(spin_strip_strainStdev_min);
        label_6->setBuddy(spin_strip_amp_min);
        label_8->setBuddy(spin_strip_per2);
        label_7->setBuddy(spin_strip_per1);
        label_31->setBuddy(spin_p_perm);
        label_30->setBuddy(spin_perm_ex);
        label_5->setBuddy(spin_perm_strainStdev_max);
        label_26->setBuddy(spin_popSize);
        label_genTotal->setBuddy(spin_contStructs);
        label_25->setBuddy(spin_p_cross);
        label_3->setBuddy(spin_cross_minimumContribution);
        label_9->setBuddy(spin_numInitial);
#endif // QT_NO_SHORTCUT
        QWidget::setTabOrder(spin_numInitial, list_seeds);
        QWidget::setTabOrder(list_seeds, push_addSeed);
        QWidget::setTabOrder(push_addSeed, push_removeSeed);
        QWidget::setTabOrder(push_removeSeed, spin_popSize);
        QWidget::setTabOrder(spin_popSize, spin_contStructs);
        QWidget::setTabOrder(spin_contStructs, cb_limitRunningJobs);
        QWidget::setTabOrder(cb_limitRunningJobs, spin_runningJobLimit);
        QWidget::setTabOrder(spin_runningJobLimit, spin_failLimit);
        QWidget::setTabOrder(spin_failLimit, combo_failAction);
        QWidget::setTabOrder(combo_failAction, spin_p_cross);
        QWidget::setTabOrder(spin_p_cross, spin_cross_minimumContribution);
        QWidget::setTabOrder(spin_cross_minimumContribution, spin_p_strip);
        QWidget::setTabOrder(spin_p_strip, spin_strip_strainStdev_min);
        QWidget::setTabOrder(spin_strip_strainStdev_min, spin_strip_amp_min);
        QWidget::setTabOrder(spin_strip_amp_min, spin_strip_amp_max);
        QWidget::setTabOrder(spin_strip_amp_max, spin_strip_strainStdev_max);
        QWidget::setTabOrder(spin_strip_strainStdev_max, spin_strip_per1);
        QWidget::setTabOrder(spin_strip_per1, spin_strip_per2);
        QWidget::setTabOrder(spin_strip_per2, spin_tol_spg);
        QWidget::setTabOrder(spin_tol_spg, push_spg_reset);
        QWidget::setTabOrder(push_spg_reset, spin_tol_xcLength);
        QWidget::setTabOrder(spin_tol_xcLength, spin_tol_xcAngle);
        QWidget::setTabOrder(spin_tol_xcAngle, push_dup_reset);
        QWidget::setTabOrder(push_dup_reset, spin_p_perm);
        QWidget::setTabOrder(spin_p_perm, spin_perm_strainStdev_max);
        QWidget::setTabOrder(spin_perm_strainStdev_max, spin_perm_ex);

        retranslateUi(Tab_Opt);
        QObject::connect(cb_limitRunningJobs, SIGNAL(toggled(bool)), spin_runningJobLimit, SLOT(setEnabled(bool)));

        QMetaObject::connectSlotsByName(Tab_Opt);
    } // setupUi

    void retranslateUi(QWidget *Tab_Opt)
    {
        Tab_Opt->setWindowTitle(QApplication::translate("Tab_Opt", "Form", 0, QApplication::UnicodeUTF8));
        groupBox_6->setTitle(QApplication::translate("Tab_Opt", "Stripple", 0, QApplication::UnicodeUTF8));
        label_29->setText(QApplication::translate("Tab_Opt", "Percent new stripple:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        label_27->setToolTip(QApplication::translate("Tab_Opt", "Standard deviation of the random elements of the strain matrix.\n"
"\n"
"Default: 0.07", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        label_27->setText(QApplication::translate("Tab_Opt", "Strain stdev range:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        spin_p_strip->setToolTip(QApplication::translate("Tab_Opt", "Percentage of offspring structures that are generated from the stripple operator.\n"
"\n"
"Default: 50%", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        spin_p_strip->setSuffix(QApplication::translate("Tab_Opt", "%", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        spin_strip_strainStdev_min->setToolTip(QApplication::translate("Tab_Opt", "Minimum standard deviation for the random elements in the strain matrix.\n"
"\n"
"See publication for details.\n"
"\n"
"Default: 0.5", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        label_6->setText(QApplication::translate("Tab_Opt", "Amplitude range:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        spin_strip_amp_min->setToolTip(QApplication::translate("Tab_Opt", "Minimum ripple amplitude.\n"
"\n"
"See publication for details.\n"
"\n"
"Default: 0.5", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        label_8->setText(QApplication::translate("Tab_Opt", "Waves in axis 2:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        spin_strip_per1->setToolTip(QApplication::translate("Tab_Opt", "Number of waves in one non-displaced direction for the ripple operator. You probably don't want to change this unless using an enormous unit cell.\n"
"\n"
"Default: 1", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        spin_strip_per2->setToolTip(QApplication::translate("Tab_Opt", "Number of waves in the other non-displaced direction for the ripple operator. You probably don't want to change this unless using an enormous unit cell.\n"
"\n"
"Default: 1", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        label_7->setText(QApplication::translate("Tab_Opt", "Waves in axis 1:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        spin_strip_strainStdev_max->setToolTip(QApplication::translate("Tab_Opt", "Maximum standard deviation for the random elements in the strain matrix.\n"
"\n"
"See publication for details.\n"
"\n"
"Default: 0.5", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        spin_strip_amp_max->setToolTip(QApplication::translate("Tab_Opt", "Maximum ripple amplitude.\n"
"\n"
"See publication for details.\n"
"\n"
"Default: 1.0", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        groupBox_7->setTitle(QApplication::translate("Tab_Opt", "Permustrain", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        label_31->setToolTip(QApplication::translate("Tab_Opt", "Percentage of offspring structures that will be generated from the permutation operator.\n"
"\n"
"The permutation operator will swap two atoms of different type within a single parent. Set to zero if there is only one atom type in the cell!\n"
"\n"
"Default: 5%", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        label_31->setText(QApplication::translate("Tab_Opt", "Percent new permustrain:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        label_30->setToolTip(QApplication::translate("Tab_Opt", "Number of separate swaps in the permutation operator.\n"
"\n"
"Default: 3", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        label_30->setText(QApplication::translate("Tab_Opt", "Number of exchanges:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        spin_p_perm->setToolTip(QApplication::translate("Tab_Opt", "Percentage of offspring structures that will be generated from the permustrain operator.\n"
"\n"
"Set to zero if there is only one atom type in the cell!\n"
"\n"
"Default: 35%", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        spin_p_perm->setSuffix(QApplication::translate("Tab_Opt", "%", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        spin_perm_ex->setToolTip(QApplication::translate("Tab_Opt", "Number of separate swaps in the permutation operator.\n"
"\n"
"Default: 4", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        label_5->setText(QApplication::translate("Tab_Opt", "Maximum strain stdev:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        spin_perm_strainStdev_max->setToolTip(QApplication::translate("Tab_Opt", "Maximum standard deviation for the random elements in the strain matrix.\n"
"\n"
"See publication for details.\n"
"\n"
"Default: 0.5", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        groupBox_4->setTitle(QApplication::translate("Tab_Opt", "Search Parameters", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        label_26->setToolTip(QApplication::translate("Tab_Opt", "Number of structures to be considered for genetic operations.\n"
"\n"
"Structures are chosen for genetic operations based on a weighted probability list, such that the lowest enthalpy structures have the greatest probability of being chosen. The number specified here is used to determine how many of the lowest enthalpy structures will have a non-zero probability.\n"
"\n"
"Default: 60", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        label_26->setText(QApplication::translate("Tab_Opt", "Pool size:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        spin_popSize->setToolTip(QApplication::translate("Tab_Opt", "Number of structures to be considered for genetic operations.\n"
"\n"
"Structures are chosen for genetic operations based on a weighted probability list, such that the lowest enthalpy structures have the greatest probability of being chosen. The number specified here is used to determine how many of the lowest enthalpy structures will have a non-zero probability.\n"
"\n"
"Default: 20", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        label_genTotal->setToolTip(QApplication::translate("Tab_Opt", "How many structures are created for population > 1. This has no effect in continuous mode.\n"
"\n"
"Default: 10", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        label_genTotal->setText(QApplication::translate("Tab_Opt", "Continuous structures:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        spin_contStructs->setToolTip(QApplication::translate("Tab_Opt", "Number of structures to be kept \"in progress\" after the initial generation.\n"
"\n"
"This should depend on the resources available. If a running job limit is set below,\n"
"\n"
"[running job limit] + 1\n"
"\n"
"is a good value here.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        cb_limitRunningJobs->setToolTip(QApplication::translate("Tab_Opt", "Check this box to set a limit on the number of jobs that are submitted for a local optimization at any given time.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        cb_limitRunningJobs->setText(QApplication::translate("Tab_Opt", "Limit running jobs?", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        spin_runningJobLimit->setToolTip(QApplication::translate("Tab_Opt", "Maximum number of simultaneous local optimizations allowed.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        spin_runningJobLimit->setSuffix(QApplication::translate("Tab_Opt", " jobs", 0, QApplication::UnicodeUTF8));
        spin_runningJobLimit->setPrefix(QApplication::translate("Tab_Opt", "Limit to ", 0, QApplication::UnicodeUTF8));
        label_4->setText(QApplication::translate("Tab_Opt", "If a job fails ", 0, QApplication::UnicodeUTF8));
        spin_failLimit->setSuffix(QApplication::translate("Tab_Opt", " times,", 0, QApplication::UnicodeUTF8));
        combo_failAction->clear();
        combo_failAction->insertItems(0, QStringList()
         << QApplication::translate("Tab_Opt", "keep trying to optimize.", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Tab_Opt", "kill the structure.", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Tab_Opt", "replace with random.", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Tab_Opt", "replace with new offspring.", 0, QApplication::UnicodeUTF8)
        );
#ifndef QT_NO_TOOLTIP
        combo_failAction->setToolTip(QApplication::translate("Tab_Opt", "Either keep trying to optimize, kill, or randomize individuals that fail more than the specified number of times.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        label_11->setText(QApplication::translate("Tab_Opt", "Total Number of Structures:", 0, QApplication::UnicodeUTF8));
        groupBox_5->setTitle(QApplication::translate("Tab_Opt", "Crossover", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        label_25->setToolTip(QApplication::translate("Tab_Opt", "Percentage of new structures to be created from the heredity operator.\n"
"\n"
"Heredity combines two random spatially coherent slabs from two unique parents to form a single offspring.\n"
"\n"
"Default: 85", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        label_25->setText(QApplication::translate("Tab_Opt", "Percent new crossover:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        spin_p_cross->setToolTip(QApplication::translate("Tab_Opt", "Percentage of new structures to be created from the crossover operator.\n"
"\n"
"Default: 15", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        spin_p_cross->setSuffix(QApplication::translate("Tab_Opt", "%", 0, QApplication::UnicodeUTF8));
        label_3->setText(QApplication::translate("Tab_Opt", "Minimum contribution:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        spin_cross_minimumContribution->setToolTip(QApplication::translate("Tab_Opt", "This is the smallest contribution a parent may make to the offspring during crossover.\n"
"\n"
"Default: 25%", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        spin_cross_minimumContribution->setSuffix(QApplication::translate("Tab_Opt", "%", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        groupBox->setToolTip(QApplication::translate("Tab_Opt", "Experimental: These options are used to fine-tune matching of duplicate structures.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        groupBox->setTitle(QApplication::translate("Tab_Opt", "Tolerances", 0, QApplication::UnicodeUTF8));
        groupBox_8->setTitle(QApplication::translate("Tab_Opt", "Spacegroup perception", 0, QApplication::UnicodeUTF8));
        label_10->setText(QApplication::translate("Tab_Opt", "Length tolerance", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        spin_tol_spg->setToolTip(QApplication::translate("Tab_Opt", "Tolerance used in spacegroup determination.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        spin_tol_spg->setSuffix(QApplication::translate("Tab_Opt", " \303\205", 0, QApplication::UnicodeUTF8));
        push_spg_reset->setText(QApplication::translate("Tab_Opt", "Re-detect spacegroups...", 0, QApplication::UnicodeUTF8));
        groupBox_9->setTitle(QApplication::translate("Tab_Opt", "Duplicate matching", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("Tab_Opt", "Length tolerance", 0, QApplication::UnicodeUTF8));
        spin_tol_xcLength->setSuffix(QApplication::translate("Tab_Opt", " \303\205", 0, QApplication::UnicodeUTF8));
        label_2->setText(QApplication::translate("Tab_Opt", "Angle tolerance", 0, QApplication::UnicodeUTF8));
        spin_tol_xcAngle->setSuffix(QApplication::translate("Tab_Opt", "\302\260", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        push_dup_reset->setToolTip(QApplication::translate("Tab_Opt", "Manually recheck all structures for duplicates (necessary after changing the above options).", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        push_dup_reset->setText(QApplication::translate("Tab_Opt", "Reset duplicates...", 0, QApplication::UnicodeUTF8));
        groupBox_3->setTitle(QApplication::translate("Tab_Opt", "Initial Generation", 0, QApplication::UnicodeUTF8));
        groupBox_2->setTitle(QApplication::translate("Tab_Opt", "Initial Seed Structures:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        list_seeds->setToolTip(QApplication::translate("Tab_Opt", "Load any initial seed structures here.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        push_addSeed->setText(QApplication::translate("Tab_Opt", "&Add", 0, QApplication::UnicodeUTF8));
        push_removeSeed->setText(QApplication::translate("Tab_Opt", "&Remove", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        label_9->setToolTip(QApplication::translate("Tab_Opt", "Number of initial structures / number of structures to keep in queue for continuous mode.\n"
"\n"
"Default: 20\n"
"", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        label_9->setText(QApplication::translate("Tab_Opt", "Initial structures:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        spin_numInitial->setToolTip(QApplication::translate("Tab_Opt", "Number of initial structures to use in first generation.\n"
"", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
    } // retranslateUi

};

namespace Ui {
    class Tab_Opt: public Ui_Tab_Opt {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TAB_OPT_H
