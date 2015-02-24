/********************************************************************************
** Form generated from reading UI file 'defaultedittab.ui'
**
** Created by: Qt User Interface Compiler version 4.8.6
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DEFAULTEDITTAB_H
#define UI_DEFAULTEDITTAB_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QComboBox>
#include <QtGui/QGridLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QListWidget>
#include <QtGui/QPushButton>
#include <QtGui/QTextEdit>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_DefaultEditTab
{
public:
    QHBoxLayout *horizontalLayout_3;
    QGridLayout *gridLayout;
    QLabel *label_6;
    QLabel *label_5;
    QLabel *label_15;
    QListWidget *list_optStep;
    QHBoxLayout *horizontalLayout;
    QPushButton *push_add;
    QPushButton *push_remove;
    QPushButton *push_help;
    QHBoxLayout *horizontalLayout_2;
    QPushButton *push_saveScheme;
    QPushButton *push_loadScheme;
    QLabel *label;
    QLabel *label_2;
    QLabel *label_3;
    QLabel *label_4;
    QLineEdit *edit_user2;
    QLineEdit *edit_user1;
    QComboBox *combo_queueInterfaces;
    QComboBox *combo_optimizers;
    QComboBox *combo_templates;
    QLineEdit *edit_user4;
    QLineEdit *edit_user3;
    QPushButton *push_queueInterfaceConfig;
    QPushButton *push_optimizerConfig;
    QTextEdit *edit_edit;
    QListWidget *list_edit;

    void setupUi(QWidget *DefaultEditTab)
    {
        if (DefaultEditTab->objectName().isEmpty())
            DefaultEditTab->setObjectName(QString::fromUtf8("DefaultEditTab"));
        DefaultEditTab->resize(969, 622);
        horizontalLayout_3 = new QHBoxLayout(DefaultEditTab);
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        gridLayout = new QGridLayout();
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        label_6 = new QLabel(DefaultEditTab);
        label_6->setObjectName(QString::fromUtf8("label_6"));
        QSizePolicy sizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(label_6->sizePolicy().hasHeightForWidth());
        label_6->setSizePolicy(sizePolicy);

        gridLayout->addWidget(label_6, 0, 0, 1, 1);

        label_5 = new QLabel(DefaultEditTab);
        label_5->setObjectName(QString::fromUtf8("label_5"));

        gridLayout->addWidget(label_5, 1, 0, 1, 1);

        label_15 = new QLabel(DefaultEditTab);
        label_15->setObjectName(QString::fromUtf8("label_15"));

        gridLayout->addWidget(label_15, 2, 0, 1, 1);

        list_optStep = new QListWidget(DefaultEditTab);
        list_optStep->setObjectName(QString::fromUtf8("list_optStep"));

        gridLayout->addWidget(list_optStep, 3, 0, 1, 3);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        push_add = new QPushButton(DefaultEditTab);
        push_add->setObjectName(QString::fromUtf8("push_add"));

        horizontalLayout->addWidget(push_add);

        push_remove = new QPushButton(DefaultEditTab);
        push_remove->setObjectName(QString::fromUtf8("push_remove"));

        horizontalLayout->addWidget(push_remove);

        push_help = new QPushButton(DefaultEditTab);
        push_help->setObjectName(QString::fromUtf8("push_help"));

        horizontalLayout->addWidget(push_help);


        gridLayout->addLayout(horizontalLayout, 4, 0, 1, 3);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        push_saveScheme = new QPushButton(DefaultEditTab);
        push_saveScheme->setObjectName(QString::fromUtf8("push_saveScheme"));

        horizontalLayout_2->addWidget(push_saveScheme);

        push_loadScheme = new QPushButton(DefaultEditTab);
        push_loadScheme->setObjectName(QString::fromUtf8("push_loadScheme"));

        horizontalLayout_2->addWidget(push_loadScheme);


        gridLayout->addLayout(horizontalLayout_2, 5, 0, 1, 3);

        label = new QLabel(DefaultEditTab);
        label->setObjectName(QString::fromUtf8("label"));

        gridLayout->addWidget(label, 6, 0, 1, 1);

        label_2 = new QLabel(DefaultEditTab);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        gridLayout->addWidget(label_2, 7, 0, 1, 1);

        label_3 = new QLabel(DefaultEditTab);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        gridLayout->addWidget(label_3, 8, 0, 1, 1);

        label_4 = new QLabel(DefaultEditTab);
        label_4->setObjectName(QString::fromUtf8("label_4"));

        gridLayout->addWidget(label_4, 9, 0, 1, 1);

        edit_user2 = new QLineEdit(DefaultEditTab);
        edit_user2->setObjectName(QString::fromUtf8("edit_user2"));

        gridLayout->addWidget(edit_user2, 7, 1, 1, 2);

        edit_user1 = new QLineEdit(DefaultEditTab);
        edit_user1->setObjectName(QString::fromUtf8("edit_user1"));

        gridLayout->addWidget(edit_user1, 6, 1, 1, 2);

        combo_queueInterfaces = new QComboBox(DefaultEditTab);
        combo_queueInterfaces->setObjectName(QString::fromUtf8("combo_queueInterfaces"));

        gridLayout->addWidget(combo_queueInterfaces, 0, 1, 1, 1);

        combo_optimizers = new QComboBox(DefaultEditTab);
        combo_optimizers->setObjectName(QString::fromUtf8("combo_optimizers"));

        gridLayout->addWidget(combo_optimizers, 1, 1, 1, 1);

        combo_templates = new QComboBox(DefaultEditTab);
        combo_templates->setObjectName(QString::fromUtf8("combo_templates"));

        gridLayout->addWidget(combo_templates, 2, 1, 1, 2);

        edit_user4 = new QLineEdit(DefaultEditTab);
        edit_user4->setObjectName(QString::fromUtf8("edit_user4"));

        gridLayout->addWidget(edit_user4, 9, 1, 1, 2);

        edit_user3 = new QLineEdit(DefaultEditTab);
        edit_user3->setObjectName(QString::fromUtf8("edit_user3"));

        gridLayout->addWidget(edit_user3, 8, 1, 1, 2);

        push_queueInterfaceConfig = new QPushButton(DefaultEditTab);
        push_queueInterfaceConfig->setObjectName(QString::fromUtf8("push_queueInterfaceConfig"));
        QSizePolicy sizePolicy1(QSizePolicy::Maximum, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(push_queueInterfaceConfig->sizePolicy().hasHeightForWidth());
        push_queueInterfaceConfig->setSizePolicy(sizePolicy1);

        gridLayout->addWidget(push_queueInterfaceConfig, 0, 2, 1, 1);

        push_optimizerConfig = new QPushButton(DefaultEditTab);
        push_optimizerConfig->setObjectName(QString::fromUtf8("push_optimizerConfig"));

        gridLayout->addWidget(push_optimizerConfig, 1, 2, 1, 1);


        horizontalLayout_3->addLayout(gridLayout);

        edit_edit = new QTextEdit(DefaultEditTab);
        edit_edit->setObjectName(QString::fromUtf8("edit_edit"));

        horizontalLayout_3->addWidget(edit_edit);

        list_edit = new QListWidget(DefaultEditTab);
        list_edit->setObjectName(QString::fromUtf8("list_edit"));

        horizontalLayout_3->addWidget(list_edit);

#ifndef QT_NO_SHORTCUT
        label_6->setBuddy(combo_queueInterfaces);
        label_5->setBuddy(combo_optimizers);
        label_15->setBuddy(combo_templates);
#endif // QT_NO_SHORTCUT
        QWidget::setTabOrder(list_optStep, edit_user2);
        QWidget::setTabOrder(edit_user2, edit_user4);
        QWidget::setTabOrder(edit_user4, edit_edit);
        QWidget::setTabOrder(edit_edit, list_edit);

        retranslateUi(DefaultEditTab);

        QMetaObject::connectSlotsByName(DefaultEditTab);
    } // setupUi

    void retranslateUi(QWidget *DefaultEditTab)
    {
        DefaultEditTab->setWindowTitle(QApplication::translate("DefaultEditTab", "Form", 0, QApplication::UnicodeUTF8));
        label_6->setText(QApplication::translate("DefaultEditTab", "&Queue", 0, QApplication::UnicodeUTF8));
        label_5->setText(QApplication::translate("DefaultEditTab", "&Optimizer", 0, QApplication::UnicodeUTF8));
        label_15->setText(QApplication::translate("DefaultEditTab", "&Template", 0, QApplication::UnicodeUTF8));
        push_add->setText(QApplication::translate("DefaultEditTab", "Add..", 0, QApplication::UnicodeUTF8));
        push_remove->setText(QApplication::translate("DefaultEditTab", "Remove...", 0, QApplication::UnicodeUTF8));
        push_help->setText(QApplication::translate("DefaultEditTab", "&Help", 0, QApplication::UnicodeUTF8));
        push_saveScheme->setText(QApplication::translate("DefaultEditTab", "&Save Opt Scheme", 0, QApplication::UnicodeUTF8));
        push_loadScheme->setText(QApplication::translate("DefaultEditTab", "&Load Opt Scheme", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("DefaultEditTab", "user1:", 0, QApplication::UnicodeUTF8));
        label_2->setText(QApplication::translate("DefaultEditTab", "user2:", 0, QApplication::UnicodeUTF8));
        label_3->setText(QApplication::translate("DefaultEditTab", "user3:", 0, QApplication::UnicodeUTF8));
        label_4->setText(QApplication::translate("DefaultEditTab", "user4:", 0, QApplication::UnicodeUTF8));
        push_queueInterfaceConfig->setText(QApplication::translate("DefaultEditTab", "Configure...", 0, QApplication::UnicodeUTF8));
        push_optimizerConfig->setText(QApplication::translate("DefaultEditTab", "Configure...", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class DefaultEditTab: public Ui_DefaultEditTab {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DEFAULTEDITTAB_H
