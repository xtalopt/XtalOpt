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
#include <QtGui/QGridLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QTableWidget>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Tab_Init
{
public:
    QGridLayout *gridLayout;
    QLabel *label;
    QLineEdit *edit_substrateFile;
    QLabel *label_2;
    QPushButton *push_substrateBrowse;
    QPushButton *push_substrateCurrent;
    QSpacerItem *horizontalSpacer;
    QPushButton *push_matrixRemove;
    QPushButton *push_matrixCurrent;
    QPushButton *push_matrixAdd;
    QTableWidget *table_matrix;

    void setupUi(QWidget *Tab_Init)
    {
        if (Tab_Init->objectName().isEmpty())
            Tab_Init->setObjectName(QString::fromUtf8("Tab_Init"));
        Tab_Init->resize(407, 157);
        gridLayout = new QGridLayout(Tab_Init);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        label = new QLabel(Tab_Init);
        label->setObjectName(QString::fromUtf8("label"));

        gridLayout->addWidget(label, 1, 0, 1, 1);

        edit_substrateFile = new QLineEdit(Tab_Init);
        edit_substrateFile->setObjectName(QString::fromUtf8("edit_substrateFile"));

        gridLayout->addWidget(edit_substrateFile, 1, 1, 1, 2);

        label_2 = new QLabel(Tab_Init);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        gridLayout->addWidget(label_2, 2, 0, 1, 3);

        push_substrateBrowse = new QPushButton(Tab_Init);
        push_substrateBrowse->setObjectName(QString::fromUtf8("push_substrateBrowse"));

        gridLayout->addWidget(push_substrateBrowse, 1, 3, 1, 1);

        push_substrateCurrent = new QPushButton(Tab_Init);
        push_substrateCurrent->setObjectName(QString::fromUtf8("push_substrateCurrent"));
        push_substrateCurrent->setEnabled(false);

        gridLayout->addWidget(push_substrateCurrent, 1, 4, 1, 1);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer, 4, 1, 1, 1);

        push_matrixRemove = new QPushButton(Tab_Init);
        push_matrixRemove->setObjectName(QString::fromUtf8("push_matrixRemove"));

        gridLayout->addWidget(push_matrixRemove, 4, 3, 1, 1);

        push_matrixCurrent = new QPushButton(Tab_Init);
        push_matrixCurrent->setObjectName(QString::fromUtf8("push_matrixCurrent"));
        push_matrixCurrent->setEnabled(false);

        gridLayout->addWidget(push_matrixCurrent, 4, 4, 1, 1);

        push_matrixAdd = new QPushButton(Tab_Init);
        push_matrixAdd->setObjectName(QString::fromUtf8("push_matrixAdd"));

        gridLayout->addWidget(push_matrixAdd, 4, 2, 1, 1);

        table_matrix = new QTableWidget(Tab_Init);
        if (table_matrix->columnCount() < 3)
            table_matrix->setColumnCount(3);
        QTableWidgetItem *__qtablewidgetitem = new QTableWidgetItem();
        table_matrix->setHorizontalHeaderItem(0, __qtablewidgetitem);
        QTableWidgetItem *__qtablewidgetitem1 = new QTableWidgetItem();
        table_matrix->setHorizontalHeaderItem(1, __qtablewidgetitem1);
        QTableWidgetItem *__qtablewidgetitem2 = new QTableWidgetItem();
        table_matrix->setHorizontalHeaderItem(2, __qtablewidgetitem2);
        table_matrix->setObjectName(QString::fromUtf8("table_matrix"));
        table_matrix->setSelectionMode(QAbstractItemView::SingleSelection);
        table_matrix->setSelectionBehavior(QAbstractItemView::SelectRows);
        table_matrix->horizontalHeader()->setStretchLastSection(true);

        gridLayout->addWidget(table_matrix, 3, 0, 1, 5);

#ifndef QT_NO_SHORTCUT
        label->setBuddy(edit_substrateFile);
#endif // QT_NO_SHORTCUT
        QWidget::setTabOrder(edit_substrateFile, push_substrateBrowse);
        QWidget::setTabOrder(push_substrateBrowse, push_substrateCurrent);
        QWidget::setTabOrder(push_substrateCurrent, table_matrix);
        QWidget::setTabOrder(table_matrix, push_matrixAdd);
        QWidget::setTabOrder(push_matrixAdd, push_matrixRemove);
        QWidget::setTabOrder(push_matrixRemove, push_matrixCurrent);

        retranslateUi(Tab_Init);

        QMetaObject::connectSlotsByName(Tab_Init);
    } // setupUi

    void retranslateUi(QWidget *Tab_Init)
    {
        Tab_Init->setWindowTitle(QApplication::translate("Tab_Init", "Form", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("Tab_Init", "&Substrate Molecule:", 0, QApplication::UnicodeUTF8));
        label_2->setText(QApplication::translate("Tab_Init", "Matrix Molecules:", 0, QApplication::UnicodeUTF8));
        push_substrateBrowse->setText(QApplication::translate("Tab_Init", "&Browse...", 0, QApplication::UnicodeUTF8));
        push_substrateCurrent->setText(QApplication::translate("Tab_Init", "Use &Current", 0, QApplication::UnicodeUTF8));
        push_matrixRemove->setText(QApplication::translate("Tab_Init", "&Remove", 0, QApplication::UnicodeUTF8));
        push_matrixCurrent->setText(QApplication::translate("Tab_Init", "Add &Current", 0, QApplication::UnicodeUTF8));
        push_matrixAdd->setText(QApplication::translate("Tab_Init", "&Add", 0, QApplication::UnicodeUTF8));
        QTableWidgetItem *___qtablewidgetitem = table_matrix->horizontalHeaderItem(0);
        ___qtablewidgetitem->setText(QApplication::translate("Tab_Init", "#", 0, QApplication::UnicodeUTF8));
        QTableWidgetItem *___qtablewidgetitem1 = table_matrix->horizontalHeaderItem(1);
        ___qtablewidgetitem1->setText(QApplication::translate("Tab_Init", "Stoich", 0, QApplication::UnicodeUTF8));
        QTableWidgetItem *___qtablewidgetitem2 = table_matrix->horizontalHeaderItem(2);
        ___qtablewidgetitem2->setText(QApplication::translate("Tab_Init", "Filename", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class Tab_Init: public Ui_Tab_Init {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TAB_INIT_H
