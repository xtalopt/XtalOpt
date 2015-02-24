/********************************************************************************
** Form generated from reading UI file 'tab_progress.ui'
**
** Created by: Qt User Interface Compiler version 4.8.6
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TAB_PROGRESS_H
#define UI_TAB_PROGRESS_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QGridLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QSpinBox>
#include <QtGui/QTableWidget>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Tab_Progress
{
public:
    QGridLayout *gridLayout;
    QTableWidget *table_list;
    QPushButton *push_refresh;
    QLabel *label_36;
    QSpinBox *spin_period;
    QSpacerItem *horizontalSpacer_5;
    QPushButton *push_print;
    QPushButton *push_clear;
    QPushButton *push_rank;
    QPushButton *push_refreshAll;

    void setupUi(QWidget *Tab_Progress)
    {
        if (Tab_Progress->objectName().isEmpty())
            Tab_Progress->setObjectName(QString::fromUtf8("Tab_Progress"));
        Tab_Progress->resize(1130, 597);
        gridLayout = new QGridLayout(Tab_Progress);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        table_list = new QTableWidget(Tab_Progress);
        if (table_list->columnCount() < 9)
            table_list->setColumnCount(9);
        QTableWidgetItem *__qtablewidgetitem = new QTableWidgetItem();
        table_list->setHorizontalHeaderItem(0, __qtablewidgetitem);
        QTableWidgetItem *__qtablewidgetitem1 = new QTableWidgetItem();
        table_list->setHorizontalHeaderItem(1, __qtablewidgetitem1);
        QTableWidgetItem *__qtablewidgetitem2 = new QTableWidgetItem();
        table_list->setHorizontalHeaderItem(2, __qtablewidgetitem2);
        QTableWidgetItem *__qtablewidgetitem3 = new QTableWidgetItem();
        table_list->setHorizontalHeaderItem(3, __qtablewidgetitem3);
        QTableWidgetItem *__qtablewidgetitem4 = new QTableWidgetItem();
        table_list->setHorizontalHeaderItem(4, __qtablewidgetitem4);
        QTableWidgetItem *__qtablewidgetitem5 = new QTableWidgetItem();
        table_list->setHorizontalHeaderItem(5, __qtablewidgetitem5);
        QTableWidgetItem *__qtablewidgetitem6 = new QTableWidgetItem();
        table_list->setHorizontalHeaderItem(6, __qtablewidgetitem6);
        QTableWidgetItem *__qtablewidgetitem7 = new QTableWidgetItem();
        table_list->setHorizontalHeaderItem(7, __qtablewidgetitem7);
        QTableWidgetItem *__qtablewidgetitem8 = new QTableWidgetItem();
        table_list->setHorizontalHeaderItem(8, __qtablewidgetitem8);
        table_list->setObjectName(QString::fromUtf8("table_list"));
        table_list->setSortingEnabled(true);
        table_list->setContextMenuPolicy(Qt::CustomContextMenu);
        table_list->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table_list->setAlternatingRowColors(true);
        table_list->setSelectionMode(QAbstractItemView::SingleSelection);
        table_list->setSelectionBehavior(QAbstractItemView::SelectRows);
        table_list->horizontalHeader()->setStretchLastSection(true);

        gridLayout->addWidget(table_list, 0, 0, 1, 8);

        push_refresh = new QPushButton(Tab_Progress);
        push_refresh->setObjectName(QString::fromUtf8("push_refresh"));
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(push_refresh->sizePolicy().hasHeightForWidth());
        push_refresh->setSizePolicy(sizePolicy);

        gridLayout->addWidget(push_refresh, 2, 0, 1, 1);

        label_36 = new QLabel(Tab_Progress);
        label_36->setObjectName(QString::fromUtf8("label_36"));

        gridLayout->addWidget(label_36, 2, 1, 1, 1);

        spin_period = new QSpinBox(Tab_Progress);
        spin_period->setObjectName(QString::fromUtf8("spin_period"));
        spin_period->setMaximum(3600);
        spin_period->setSingleStep(1);
        spin_period->setValue(1);

        gridLayout->addWidget(spin_period, 2, 2, 1, 1);

        horizontalSpacer_5 = new QSpacerItem(5, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer_5, 2, 3, 1, 1);

        push_print = new QPushButton(Tab_Progress);
        push_print->setObjectName(QString::fromUtf8("push_print"));

        gridLayout->addWidget(push_print, 2, 4, 1, 1);

        push_clear = new QPushButton(Tab_Progress);
        push_clear->setObjectName(QString::fromUtf8("push_clear"));

        gridLayout->addWidget(push_clear, 2, 5, 1, 1);

        push_rank = new QPushButton(Tab_Progress);
        push_rank->setObjectName(QString::fromUtf8("push_rank"));

        gridLayout->addWidget(push_rank, 2, 6, 1, 1);

        push_refreshAll = new QPushButton(Tab_Progress);
        push_refreshAll->setObjectName(QString::fromUtf8("push_refreshAll"));

        gridLayout->addWidget(push_refreshAll, 2, 7, 1, 1);

#ifndef QT_NO_SHORTCUT
        label_36->setBuddy(spin_period);
#endif // QT_NO_SHORTCUT
        QWidget::setTabOrder(table_list, push_clear);
        QWidget::setTabOrder(push_clear, spin_period);

        retranslateUi(Tab_Progress);

        QMetaObject::connectSlotsByName(Tab_Progress);
    } // setupUi

    void retranslateUi(QWidget *Tab_Progress)
    {
        Tab_Progress->setWindowTitle(QApplication::translate("Tab_Progress", "Form", 0, QApplication::UnicodeUTF8));
        QTableWidgetItem *___qtablewidgetitem = table_list->horizontalHeaderItem(0);
        ___qtablewidgetitem->setText(QApplication::translate("Tab_Progress", "Gen", 0, QApplication::UnicodeUTF8));
        QTableWidgetItem *___qtablewidgetitem1 = table_list->horizontalHeaderItem(1);
        ___qtablewidgetitem1->setText(QApplication::translate("Tab_Progress", "Mol", 0, QApplication::UnicodeUTF8));
        QTableWidgetItem *___qtablewidgetitem2 = table_list->horizontalHeaderItem(2);
        ___qtablewidgetitem2->setText(QApplication::translate("Tab_Progress", "Job ID", 0, QApplication::UnicodeUTF8));
        QTableWidgetItem *___qtablewidgetitem3 = table_list->horizontalHeaderItem(3);
        ___qtablewidgetitem3->setText(QApplication::translate("Tab_Progress", "Status", 0, QApplication::UnicodeUTF8));
        QTableWidgetItem *___qtablewidgetitem4 = table_list->horizontalHeaderItem(4);
        ___qtablewidgetitem4->setText(QApplication::translate("Tab_Progress", "Time Elapsed", 0, QApplication::UnicodeUTF8));
        QTableWidgetItem *___qtablewidgetitem5 = table_list->horizontalHeaderItem(5);
        ___qtablewidgetitem5->setText(QApplication::translate("Tab_Progress", "Enthalpy (eV)", 0, QApplication::UnicodeUTF8));
        QTableWidgetItem *___qtablewidgetitem6 = table_list->horizontalHeaderItem(6);
        ___qtablewidgetitem6->setText(QApplication::translate("Tab_Progress", "Volume", 0, QApplication::UnicodeUTF8));
        QTableWidgetItem *___qtablewidgetitem7 = table_list->horizontalHeaderItem(7);
        ___qtablewidgetitem7->setText(QApplication::translate("Tab_Progress", "Space Group", 0, QApplication::UnicodeUTF8));
        QTableWidgetItem *___qtablewidgetitem8 = table_list->horizontalHeaderItem(8);
        ___qtablewidgetitem8->setText(QApplication::translate("Tab_Progress", "Ancestry", 0, QApplication::UnicodeUTF8));
        push_refresh->setText(QApplication::translate("Tab_Progress", "&Refresh", 0, QApplication::UnicodeUTF8));
        label_36->setText(QApplication::translate("Tab_Progress", "Refresh &period:", 0, QApplication::UnicodeUTF8));
        spin_period->setSuffix(QApplication::translate("Tab_Progress", " seconds", 0, QApplication::UnicodeUTF8));
        push_print->setText(QApplication::translate("Tab_Progress", "Pri&nt File", 0, QApplication::UnicodeUTF8));
        push_clear->setText(QApplication::translate("Tab_Progress", "Remov&e Extra Files", 0, QApplication::UnicodeUTF8));
        push_rank->setText(QApplication::translate("Tab_Progress", "Ran&k all", 0, QApplication::UnicodeUTF8));
        push_refreshAll->setText(QApplication::translate("Tab_Progress", "Refresh &all", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class Tab_Progress: public Ui_Tab_Progress {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TAB_PROGRESS_H
