/********************************************************************************
** Form generated from reading UI file 'tab_params.ui'
**
** Created by: Qt User Interface Compiler version 4.8.6
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TAB_PARAMS_H
#define UI_TAB_PARAMS_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QDoubleSpinBox>
#include <QtGui/QGridLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QSpacerItem>
#include <QtGui/QSpinBox>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Tab_Params
{
public:
    QGridLayout *gridLayout;
    QLabel *label;
    QLabel *label_2;
    QLabel *label_3;
    QLabel *label_5;
    QSpinBox *spin_numSearches;
    QSpinBox *spin_numMatrixMols;
    QDoubleSpinBox *spin_IAD_min;
    QDoubleSpinBox *spin_IAD_max;
    QDoubleSpinBox *spin_radius_min;
    QDoubleSpinBox *spin_radius_max;
    QSpacerItem *verticalSpacer;
    QCheckBox *cb_radius_auto;
    QSpacerItem *horizontalSpacer;
    QLabel *label_4;
    QSpinBox *spin_cutoff;
    QCheckBox *cb_cluster;
    QCheckBox *cb_strictHBonds;
    QCheckBox *cb_build2DNetwork;

    void setupUi(QWidget *Tab_Params)
    {
        if (Tab_Params->objectName().isEmpty())
            Tab_Params->setObjectName(QString::fromUtf8("Tab_Params"));
        Tab_Params->resize(529, 213);
        gridLayout = new QGridLayout(Tab_Params);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        label = new QLabel(Tab_Params);
        label->setObjectName(QString::fromUtf8("label"));

        gridLayout->addWidget(label, 2, 0, 1, 1);

        label_2 = new QLabel(Tab_Params);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        gridLayout->addWidget(label_2, 0, 0, 1, 1);

        label_3 = new QLabel(Tab_Params);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        gridLayout->addWidget(label_3, 4, 0, 1, 1);

        label_5 = new QLabel(Tab_Params);
        label_5->setObjectName(QString::fromUtf8("label_5"));

        gridLayout->addWidget(label_5, 5, 0, 1, 1);

        spin_numSearches = new QSpinBox(Tab_Params);
        spin_numSearches->setObjectName(QString::fromUtf8("spin_numSearches"));
        spin_numSearches->setMaximum(10000);

        gridLayout->addWidget(spin_numSearches, 0, 1, 1, 1);

        spin_numMatrixMols = new QSpinBox(Tab_Params);
        spin_numMatrixMols->setObjectName(QString::fromUtf8("spin_numMatrixMols"));

        gridLayout->addWidget(spin_numMatrixMols, 2, 1, 1, 1);

        spin_IAD_min = new QDoubleSpinBox(Tab_Params);
        spin_IAD_min->setObjectName(QString::fromUtf8("spin_IAD_min"));

        gridLayout->addWidget(spin_IAD_min, 4, 1, 1, 1);

        spin_IAD_max = new QDoubleSpinBox(Tab_Params);
        spin_IAD_max->setObjectName(QString::fromUtf8("spin_IAD_max"));

        gridLayout->addWidget(spin_IAD_max, 4, 2, 1, 1);

        spin_radius_min = new QDoubleSpinBox(Tab_Params);
        spin_radius_min->setObjectName(QString::fromUtf8("spin_radius_min"));

        gridLayout->addWidget(spin_radius_min, 5, 1, 1, 1);

        spin_radius_max = new QDoubleSpinBox(Tab_Params);
        spin_radius_max->setObjectName(QString::fromUtf8("spin_radius_max"));

        gridLayout->addWidget(spin_radius_max, 5, 2, 1, 1);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(verticalSpacer, 8, 0, 1, 1);

        cb_radius_auto = new QCheckBox(Tab_Params);
        cb_radius_auto->setObjectName(QString::fromUtf8("cb_radius_auto"));

        gridLayout->addWidget(cb_radius_auto, 5, 3, 1, 1);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer, 8, 3, 1, 1);

        label_4 = new QLabel(Tab_Params);
        label_4->setObjectName(QString::fromUtf8("label_4"));

        gridLayout->addWidget(label_4, 1, 0, 1, 1);

        spin_cutoff = new QSpinBox(Tab_Params);
        spin_cutoff->setObjectName(QString::fromUtf8("spin_cutoff"));
        spin_cutoff->setMaximum(10000);

        gridLayout->addWidget(spin_cutoff, 1, 1, 1, 1);

        cb_cluster = new QCheckBox(Tab_Params);
        cb_cluster->setObjectName(QString::fromUtf8("cb_cluster"));

        gridLayout->addWidget(cb_cluster, 6, 0, 1, 1);

        cb_strictHBonds = new QCheckBox(Tab_Params);
        cb_strictHBonds->setObjectName(QString::fromUtf8("cb_strictHBonds"));

        gridLayout->addWidget(cb_strictHBonds, 7, 0, 1, 1);

        cb_build2DNetwork = new QCheckBox(Tab_Params);
        cb_build2DNetwork->setObjectName(QString::fromUtf8("cb_build2DNetwork"));

        gridLayout->addWidget(cb_build2DNetwork, 8, 0, 1, 1);

#ifndef QT_NO_SHORTCUT
        label->setBuddy(spin_numMatrixMols);
        label_2->setBuddy(spin_numSearches);
        label_3->setBuddy(spin_IAD_min);
        label_5->setBuddy(spin_radius_min);
#endif // QT_NO_SHORTCUT
        QWidget::setTabOrder(spin_numSearches, spin_cutoff);
        QWidget::setTabOrder(spin_cutoff, spin_numMatrixMols);
        QWidget::setTabOrder(spin_numMatrixMols, spin_IAD_min);
        QWidget::setTabOrder(spin_IAD_min, spin_IAD_max);
        QWidget::setTabOrder(spin_IAD_max, spin_radius_min);
        QWidget::setTabOrder(spin_radius_min, spin_radius_max);
        QWidget::setTabOrder(spin_radius_max, cb_radius_auto);
        QWidget::setTabOrder(cb_radius_auto, cb_cluster);

        retranslateUi(Tab_Params);
        QObject::connect(cb_radius_auto, SIGNAL(clicked(bool)), spin_radius_min, SLOT(setDisabled(bool)));
        QObject::connect(cb_radius_auto, SIGNAL(clicked(bool)), spin_radius_max, SLOT(setDisabled(bool)));

        QMetaObject::connectSlotsByName(Tab_Params);
    } // setupUi

    void retranslateUi(QWidget *Tab_Params)
    {
        Tab_Params->setWindowTitle(QApplication::translate("Tab_Params", "Form", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("Tab_Params", "Number of &matrix molecules:", 0, QApplication::UnicodeUTF8));
        label_2->setText(QApplication::translate("Tab_Params", "Number of &simultaneous searches:", 0, QApplication::UnicodeUTF8));
        label_3->setText(QApplication::translate("Tab_Params", "Limits for IAD between matrix and &center molecule:", 0, QApplication::UnicodeUTF8));
        label_5->setText(QApplication::translate("Tab_Params", "Limits for matrix &radius coordinate:", 0, QApplication::UnicodeUTF8));
        cb_radius_auto->setText(QApplication::translate("Tab_Params", "Auto", 0, QApplication::UnicodeUTF8));
        label_4->setText(QApplication::translate("Tab_Params", "Final number of scenes:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        cb_cluster->setToolTip(QApplication::translate("Tab_Params", "If selected, a cluster of Matrix molecules will be built, optionally around the substrate molecule.\n"
"\n"
"The matrix radius coordinate limits will be ignored, and the IAD range above will also be used to deterimine the distance between neighboring matrix molecules.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        cb_cluster->setText(QApplication::translate("Tab_Params", "Build cluster", 0, QApplication::UnicodeUTF8));
        cb_strictHBonds->setText(QApplication::translate("Tab_Params", "Force hydrogen bonds", 0, QApplication::UnicodeUTF8));
        cb_build2DNetwork->setText(QApplication::translate("Tab_Params", "Build 2D Network", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class Tab_Params: public Ui_Tab_Params {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TAB_PARAMS_H
