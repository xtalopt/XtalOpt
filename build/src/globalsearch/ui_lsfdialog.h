/********************************************************************************
** Form generated from reading UI file 'lsfdialog.ui'
**
** Created by: Qt User Interface Compiler version 4.8.6
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_LSFDIALOG_H
#define UI_LSFDIALOG_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QGridLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QSpacerItem>
#include <QtGui/QSpinBox>

QT_BEGIN_NAMESPACE

class Ui_LsfConfigDialog
{
public:
    QGridLayout *gridLayout;
    QLabel *label_18;
    QLineEdit *edit_host;
    QSpinBox *spin_port;
    QLabel *label_19;
    QLineEdit *edit_username;
    QLabel *label_20;
    QLineEdit *edit_rempath;
    QLabel *label_21;
    QLineEdit *edit_locpath;
    QLabel *label_launch;
    QLineEdit *edit_bsub;
    QLabel *label_bkill;
    QLabel *label_check;
    QLineEdit *edit_bkill;
    QLineEdit *edit_bjobs;
    QLabel *label;
    QLineEdit *edit_description;
    QDialogButtonBox *buttonBox;
    QSpacerItem *verticalSpacer;
    QCheckBox *cb_cleanRemoteOnStop;

    void setupUi(QDialog *LsfConfigDialog)
    {
        if (LsfConfigDialog->objectName().isEmpty())
            LsfConfigDialog->setObjectName(QString::fromUtf8("LsfConfigDialog"));
        LsfConfigDialog->resize(528, 245);
        QSizePolicy sizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(LsfConfigDialog->sizePolicy().hasHeightForWidth());
        LsfConfigDialog->setSizePolicy(sizePolicy);
        gridLayout = new QGridLayout(LsfConfigDialog);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        label_18 = new QLabel(LsfConfigDialog);
        label_18->setObjectName(QString::fromUtf8("label_18"));
        sizePolicy.setHeightForWidth(label_18->sizePolicy().hasHeightForWidth());
        label_18->setSizePolicy(sizePolicy);

        gridLayout->addWidget(label_18, 0, 0, 1, 1);

        edit_host = new QLineEdit(LsfConfigDialog);
        edit_host->setObjectName(QString::fromUtf8("edit_host"));

        gridLayout->addWidget(edit_host, 0, 2, 1, 1);

        spin_port = new QSpinBox(LsfConfigDialog);
        spin_port->setObjectName(QString::fromUtf8("spin_port"));
        spin_port->setMaximum(99999);
        spin_port->setValue(22);

        gridLayout->addWidget(spin_port, 0, 3, 1, 1);

        label_19 = new QLabel(LsfConfigDialog);
        label_19->setObjectName(QString::fromUtf8("label_19"));
        sizePolicy.setHeightForWidth(label_19->sizePolicy().hasHeightForWidth());
        label_19->setSizePolicy(sizePolicy);

        gridLayout->addWidget(label_19, 1, 0, 1, 1);

        edit_username = new QLineEdit(LsfConfigDialog);
        edit_username->setObjectName(QString::fromUtf8("edit_username"));

        gridLayout->addWidget(edit_username, 1, 2, 1, 2);

        label_20 = new QLabel(LsfConfigDialog);
        label_20->setObjectName(QString::fromUtf8("label_20"));
        sizePolicy.setHeightForWidth(label_20->sizePolicy().hasHeightForWidth());
        label_20->setSizePolicy(sizePolicy);

        gridLayout->addWidget(label_20, 2, 0, 1, 2);

        edit_rempath = new QLineEdit(LsfConfigDialog);
        edit_rempath->setObjectName(QString::fromUtf8("edit_rempath"));

        gridLayout->addWidget(edit_rempath, 2, 2, 1, 2);

        label_21 = new QLabel(LsfConfigDialog);
        label_21->setObjectName(QString::fromUtf8("label_21"));
        sizePolicy.setHeightForWidth(label_21->sizePolicy().hasHeightForWidth());
        label_21->setSizePolicy(sizePolicy);

        gridLayout->addWidget(label_21, 3, 0, 1, 1);

        edit_locpath = new QLineEdit(LsfConfigDialog);
        edit_locpath->setObjectName(QString::fromUtf8("edit_locpath"));

        gridLayout->addWidget(edit_locpath, 3, 2, 1, 2);

        label_launch = new QLabel(LsfConfigDialog);
        label_launch->setObjectName(QString::fromUtf8("label_launch"));
        sizePolicy.setHeightForWidth(label_launch->sizePolicy().hasHeightForWidth());
        label_launch->setSizePolicy(sizePolicy);

        gridLayout->addWidget(label_launch, 5, 0, 1, 1);

        edit_bsub = new QLineEdit(LsfConfigDialog);
        edit_bsub->setObjectName(QString::fromUtf8("edit_bsub"));

        gridLayout->addWidget(edit_bsub, 5, 2, 1, 2);

        label_bkill = new QLabel(LsfConfigDialog);
        label_bkill->setObjectName(QString::fromUtf8("label_bkill"));
        sizePolicy.setHeightForWidth(label_bkill->sizePolicy().hasHeightForWidth());
        label_bkill->setSizePolicy(sizePolicy);

        gridLayout->addWidget(label_bkill, 6, 0, 1, 1);

        label_check = new QLabel(LsfConfigDialog);
        label_check->setObjectName(QString::fromUtf8("label_check"));
        sizePolicy.setHeightForWidth(label_check->sizePolicy().hasHeightForWidth());
        label_check->setSizePolicy(sizePolicy);

        gridLayout->addWidget(label_check, 7, 0, 1, 1);

        edit_bkill = new QLineEdit(LsfConfigDialog);
        edit_bkill->setObjectName(QString::fromUtf8("edit_bkill"));

        gridLayout->addWidget(edit_bkill, 6, 2, 1, 2);

        edit_bjobs = new QLineEdit(LsfConfigDialog);
        edit_bjobs->setObjectName(QString::fromUtf8("edit_bjobs"));

        gridLayout->addWidget(edit_bjobs, 7, 2, 1, 2);

        label = new QLabel(LsfConfigDialog);
        label->setObjectName(QString::fromUtf8("label"));

        gridLayout->addWidget(label, 4, 0, 1, 1);

        edit_description = new QLineEdit(LsfConfigDialog);
        edit_description->setObjectName(QString::fromUtf8("edit_description"));

        gridLayout->addWidget(edit_description, 4, 2, 1, 2);

        buttonBox = new QDialogButtonBox(LsfConfigDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        gridLayout->addWidget(buttonBox, 10, 3, 1, 1);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(verticalSpacer, 8, 3, 1, 1);

        cb_cleanRemoteOnStop = new QCheckBox(LsfConfigDialog);
        cb_cleanRemoteOnStop->setObjectName(QString::fromUtf8("cb_cleanRemoteOnStop"));

        gridLayout->addWidget(cb_cleanRemoteOnStop, 9, 0, 1, 4);

#ifndef QT_NO_SHORTCUT
        label_18->setBuddy(edit_host);
        label_19->setBuddy(edit_username);
        label_20->setBuddy(edit_rempath);
        label_21->setBuddy(edit_locpath);
        label_launch->setBuddy(edit_bsub);
        label_bkill->setBuddy(edit_bkill);
        label_check->setBuddy(edit_bjobs);
        label->setBuddy(edit_description);
#endif // QT_NO_SHORTCUT
        QWidget::setTabOrder(edit_host, spin_port);
        QWidget::setTabOrder(spin_port, edit_username);
        QWidget::setTabOrder(edit_username, edit_rempath);
        QWidget::setTabOrder(edit_rempath, edit_locpath);
        QWidget::setTabOrder(edit_locpath, edit_description);
        QWidget::setTabOrder(edit_description, edit_bsub);
        QWidget::setTabOrder(edit_bsub, edit_bkill);
        QWidget::setTabOrder(edit_bkill, edit_bjobs);
        QWidget::setTabOrder(edit_bjobs, cb_cleanRemoteOnStop);
        QWidget::setTabOrder(cb_cleanRemoteOnStop, buttonBox);

        retranslateUi(LsfConfigDialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), LsfConfigDialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), LsfConfigDialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(LsfConfigDialog);
    } // setupUi

    void retranslateUi(QDialog *LsfConfigDialog)
    {
        LsfConfigDialog->setWindowTitle(QApplication::translate("LsfConfigDialog", "LSF Queue Configuration", 0, QApplication::UnicodeUTF8));
        label_18->setText(QApplication::translate("LsfConfigDialog", "Host:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        edit_host->setToolTip(QApplication::translate("LsfConfigDialog", "Address of host. Can use IP or host name.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        spin_port->setPrefix(QApplication::translate("LsfConfigDialog", "SSH Port ", 0, QApplication::UnicodeUTF8));
        label_19->setText(QApplication::translate("LsfConfigDialog", "User:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        edit_username->setToolTip(QApplication::translate("LsfConfigDialog", "Username on above host.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        label_20->setText(QApplication::translate("LsfConfigDialog", "Working directory (Server):", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        edit_rempath->setToolTip(QApplication::translate("LsfConfigDialog", "Path on remote host to use during optimizations. Do not use wildcard characters or BASH-specific characters (e.g. '~' in place of /home/user).", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        label_21->setText(QApplication::translate("LsfConfigDialog", "Working directory (Local):", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        edit_locpath->setToolTip(QApplication::translate("LsfConfigDialog", "Local path to store files", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        label_launch->setToolTip(QApplication::translate("LsfConfigDialog", "Path to the LSF bsub command. Usually bsub.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        label_launch->setText(QApplication::translate("LsfConfigDialog", "Path to bsub:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        edit_bsub->setToolTip(QApplication::translate("LsfConfigDialog", "Path to the LSF bsub command. Usually bsub.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        edit_bsub->setText(QApplication::translate("LsfConfigDialog", "bsub", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        label_bkill->setToolTip(QApplication::translate("LsfConfigDialog", "Path to the LSF bkill command. Usually bkill.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        label_bkill->setText(QApplication::translate("LsfConfigDialog", "Path to bkill:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        label_check->setToolTip(QApplication::translate("LsfConfigDialog", "Path to the LSF qjobs command. Usually bjobs.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        label_check->setText(QApplication::translate("LsfConfigDialog", "Path to bjobs:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        edit_bkill->setToolTip(QApplication::translate("LsfConfigDialog", "Path to the LSF bkill command. Usually bkill.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        edit_bkill->setText(QApplication::translate("LsfConfigDialog", "bkill", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        edit_bjobs->setToolTip(QApplication::translate("LsfConfigDialog", "Path to the LSF bjobs command. Usually bjobs.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        edit_bjobs->setText(QApplication::translate("LsfConfigDialog", "bjobs", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("LsfConfigDialog", "Description:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        edit_description->setToolTip(QApplication::translate("LsfConfigDialog", "Short description of optimization (used as %description% template keyword in input templates).", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        cb_cleanRemoteOnStop->setText(QApplication::translate("LsfConfigDialog", "Clean remote directories when finished", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class LsfConfigDialog: public Ui_LsfConfigDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_LSFDIALOG_H
