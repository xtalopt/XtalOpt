/********************************************************************************
** Form generated from reading UI file 'tab_plot.ui'
**
** Created by: Qt User Interface Compiler version 4.8.6
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TAB_PLOT_H
#define UI_TAB_PLOT_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QGridLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QWidget>
#include <avogadro/plotwidget.h>

QT_BEGIN_NAMESPACE

class Ui_Tab_Plot
{
public:
    QGridLayout *gridLayout;
    QPushButton *push_refresh;
    QSpacerItem *horizontalSpacer_3;
    QGroupBox *groupBox;
    QGridLayout *gridLayout_2;
    QSpacerItem *verticalSpacer;
    QGroupBox *gb_trend;
    QGridLayout *gridLayout_3;
    QLabel *label_34;
    QComboBox *combo_yAxis;
    QLabel *label_23;
    QComboBox *combo_xAxis;
    QCheckBox *cb_showDuplicates;
    QCheckBox *cb_showIncompletes;
    QCheckBox *cb_labelPoints;
    QComboBox *combo_labelType;
    QGroupBox *gb_distance;
    QGridLayout *gridLayout_4;
    QLabel *label_2;
    QComboBox *combo_distHistXtal;
    QComboBox *combo_plotType;
    QLabel *label;
    QPushButton *pushButton;
    Avogadro::PlotWidget *plot_plot;

    void setupUi(QWidget *Tab_Plot)
    {
        if (Tab_Plot->objectName().isEmpty())
            Tab_Plot->setObjectName(QString::fromUtf8("Tab_Plot"));
        Tab_Plot->resize(1075, 577);
        gridLayout = new QGridLayout(Tab_Plot);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        push_refresh = new QPushButton(Tab_Plot);
        push_refresh->setObjectName(QString::fromUtf8("push_refresh"));

        gridLayout->addWidget(push_refresh, 1, 0, 1, 1);

        horizontalSpacer_3 = new QSpacerItem(650, 24, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer_3, 1, 1, 1, 2);

        groupBox = new QGroupBox(Tab_Plot);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        gridLayout_2 = new QGridLayout(groupBox);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout_2->addItem(verticalSpacer, 6, 0, 1, 2);

        gb_trend = new QGroupBox(groupBox);
        gb_trend->setObjectName(QString::fromUtf8("gb_trend"));
        gridLayout_3 = new QGridLayout(gb_trend);
        gridLayout_3->setObjectName(QString::fromUtf8("gridLayout_3"));
        label_34 = new QLabel(gb_trend);
        label_34->setObjectName(QString::fromUtf8("label_34"));

        gridLayout_3->addWidget(label_34, 0, 0, 1, 1);

        combo_yAxis = new QComboBox(gb_trend);
        combo_yAxis->setObjectName(QString::fromUtf8("combo_yAxis"));

        gridLayout_3->addWidget(combo_yAxis, 0, 1, 1, 1);

        label_23 = new QLabel(gb_trend);
        label_23->setObjectName(QString::fromUtf8("label_23"));

        gridLayout_3->addWidget(label_23, 1, 0, 1, 1);

        combo_xAxis = new QComboBox(gb_trend);
        combo_xAxis->setObjectName(QString::fromUtf8("combo_xAxis"));

        gridLayout_3->addWidget(combo_xAxis, 1, 1, 1, 1);

        cb_showDuplicates = new QCheckBox(gb_trend);
        cb_showDuplicates->setObjectName(QString::fromUtf8("cb_showDuplicates"));

        gridLayout_3->addWidget(cb_showDuplicates, 2, 0, 1, 2);

        cb_showIncompletes = new QCheckBox(gb_trend);
        cb_showIncompletes->setObjectName(QString::fromUtf8("cb_showIncompletes"));

        gridLayout_3->addWidget(cb_showIncompletes, 3, 0, 1, 2);

        cb_labelPoints = new QCheckBox(gb_trend);
        cb_labelPoints->setObjectName(QString::fromUtf8("cb_labelPoints"));

        gridLayout_3->addWidget(cb_labelPoints, 4, 0, 1, 1);

        combo_labelType = new QComboBox(gb_trend);
        combo_labelType->setObjectName(QString::fromUtf8("combo_labelType"));

        gridLayout_3->addWidget(combo_labelType, 4, 1, 1, 1);


        gridLayout_2->addWidget(gb_trend, 4, 0, 1, 2);

        gb_distance = new QGroupBox(groupBox);
        gb_distance->setObjectName(QString::fromUtf8("gb_distance"));
        gridLayout_4 = new QGridLayout(gb_distance);
        gridLayout_4->setObjectName(QString::fromUtf8("gridLayout_4"));
        label_2 = new QLabel(gb_distance);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        gridLayout_4->addWidget(label_2, 0, 0, 1, 1);

        combo_distHistXtal = new QComboBox(gb_distance);
        combo_distHistXtal->setObjectName(QString::fromUtf8("combo_distHistXtal"));

        gridLayout_4->addWidget(combo_distHistXtal, 0, 1, 1, 1);


        gridLayout_2->addWidget(gb_distance, 5, 0, 1, 2);

        combo_plotType = new QComboBox(groupBox);
        combo_plotType->setObjectName(QString::fromUtf8("combo_plotType"));

        gridLayout_2->addWidget(combo_plotType, 0, 1, 1, 1);

        label = new QLabel(groupBox);
        label->setObjectName(QString::fromUtf8("label"));

        gridLayout_2->addWidget(label, 0, 0, 1, 1);


        gridLayout->addWidget(groupBox, 0, 4, 1, 1);

        pushButton = new QPushButton(Tab_Plot);
        pushButton->setObjectName(QString::fromUtf8("pushButton"));
        pushButton->setCheckable(true);
        pushButton->setChecked(true);

        gridLayout->addWidget(pushButton, 1, 3, 1, 1);

        plot_plot = new Avogadro::PlotWidget(Tab_Plot);
        plot_plot->setObjectName(QString::fromUtf8("plot_plot"));
        plot_plot->setFrameShape(QFrame::StyledPanel);
        plot_plot->setFrameShadow(QFrame::Raised);

        gridLayout->addWidget(plot_plot, 0, 0, 1, 4);

#ifndef QT_NO_SHORTCUT
        label_34->setBuddy(combo_yAxis);
        label_23->setBuddy(combo_xAxis);
        label_2->setBuddy(combo_distHistXtal);
        label->setBuddy(combo_plotType);
#endif // QT_NO_SHORTCUT

        retranslateUi(Tab_Plot);
        QObject::connect(pushButton, SIGNAL(toggled(bool)), groupBox, SLOT(setVisible(bool)));

        QMetaObject::connectSlotsByName(Tab_Plot);
    } // setupUi

    void retranslateUi(QWidget *Tab_Plot)
    {
        Tab_Plot->setWindowTitle(QApplication::translate("Tab_Plot", "Form", 0, QApplication::UnicodeUTF8));
        push_refresh->setText(QApplication::translate("Tab_Plot", "&Refresh", 0, QApplication::UnicodeUTF8));
        groupBox->setTitle(QApplication::translate("Tab_Plot", "Options", 0, QApplication::UnicodeUTF8));
        gb_trend->setTitle(QApplication::translate("Tab_Plot", "Trend Plot Options", 0, QApplication::UnicodeUTF8));
        label_34->setText(QApplication::translate("Tab_Plot", "&Y axis label:", 0, QApplication::UnicodeUTF8));
        combo_yAxis->clear();
        combo_yAxis->insertItems(0, QStringList()
         << QApplication::translate("Tab_Plot", "Structure", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Tab_Plot", "Generation", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Tab_Plot", "Enthalpy (eV)", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Tab_Plot", "Energy (eV)", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Tab_Plot", "PV enthalpy term (eV)", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Tab_Plot", "A", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Tab_Plot", "B", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Tab_Plot", "C", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Tab_Plot", "\316\261", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Tab_Plot", "\316\262", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Tab_Plot", "\316\263", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Tab_Plot", "Volume", 0, QApplication::UnicodeUTF8)
        );
        label_23->setText(QApplication::translate("Tab_Plot", "&X axis label", 0, QApplication::UnicodeUTF8));
        combo_xAxis->clear();
        combo_xAxis->insertItems(0, QStringList()
         << QApplication::translate("Tab_Plot", "Structure", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Tab_Plot", "Generation", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Tab_Plot", "Enthalpy (eV)", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Tab_Plot", "Energy (eV)", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Tab_Plot", "PV enthalpy term (eV)", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Tab_Plot", "A", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Tab_Plot", "B", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Tab_Plot", "C", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Tab_Plot", "\316\261", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Tab_Plot", "\316\262", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Tab_Plot", "\316\263", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Tab_Plot", "Volume", 0, QApplication::UnicodeUTF8)
        );
        cb_showDuplicates->setText(QApplication::translate("Tab_Plot", "Show &duplicate structures", 0, QApplication::UnicodeUTF8));
        cb_showIncompletes->setText(QApplication::translate("Tab_Plot", "Show &incomplete structures", 0, QApplication::UnicodeUTF8));
        cb_labelPoints->setText(QApplication::translate("Tab_Plot", "&Label points", 0, QApplication::UnicodeUTF8));
        combo_labelType->clear();
        combo_labelType->insertItems(0, QStringList()
         << QApplication::translate("Tab_Plot", "SG Number", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Tab_Plot", "SG Symbol", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Tab_Plot", "Enthalpy (eV)", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Tab_Plot", "Energy (eV)", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Tab_Plot", "PV enthalpy term (eV)", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Tab_Plot", "Volume", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Tab_Plot", "Generation", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Tab_Plot", "Structure #", 0, QApplication::UnicodeUTF8)
        );
        gb_distance->setTitle(QApplication::translate("Tab_Plot", "Structure selection:", 0, QApplication::UnicodeUTF8));
        label_2->setText(QApplication::translate("Tab_Plot", "&Structure:", 0, QApplication::UnicodeUTF8));
        combo_plotType->clear();
        combo_plotType->insertItems(0, QStringList()
         << QApplication::translate("Tab_Plot", "Trends", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Tab_Plot", "Distance Histogram", 0, QApplication::UnicodeUTF8)
        );
        label->setText(QApplication::translate("Tab_Plot", "Plot &Type:", 0, QApplication::UnicodeUTF8));
        pushButton->setText(QApplication::translate("Tab_Plot", "&Toggle options...", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class Tab_Plot: public Ui_Tab_Plot {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TAB_PLOT_H
