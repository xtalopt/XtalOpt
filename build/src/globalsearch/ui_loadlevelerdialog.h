/********************************************************************************
** Form generated from reading UI file 'loadlevelerdialog.ui'
**
** Created by: Qt User Interface Compiler version 4.8.6
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_LOADLEVELERDIALOG_H
#define UI_LOADLEVELERDIALOG_H

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

class Ui_LoadLevelerConfigDialog
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
    QLineEdit *edit_llsubmit;
    QLabel *label_qdel;
    QLabel *label_check;
    QLineEdit *edit_llcancel;
    QLineEdit *edit_llq;
    QLabel *label;
    QLineEdit *edit_description;
    QDialogButtonBox *buttonBox;
    QSpacerItem *verticalSpacer;
    QLabel *label_2;
    QSpinBox *spin_interval;
    QCheckBox *cb_cleanRemoteOnStop;

    void setupUi(QDialog *LoadLevelerConfigDialog)
    {
        if (LoadLevelerConfigDialog->objectName().isEmpty())
            LoadLevelerConfigDialog->setObjectName(QString::fromUtf8("LoadLevelerConfigDialog"));
        LoadLevelerConfigDialog->resize(528, 296);
        QSizePolicy sizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(LoadLevelerConfigDialog->sizePolicy().hasHeightForWidth());
        LoadLevelerConfigDialog->setSizePolicy(sizePolicy);
        gridLayout = new QGridLayout(LoadLevelerConfigDialog);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        label_18 = new QLabel(LoadLevelerConfigDialog);
        label_18->setObjectName(QString::fromUtf8("label_18"));
        sizePolicy.setHeightForWidth(label_18->sizePolicy().hasHeightForWidth());
        label_18->setSizePolicy(sizePolicy);

        gridLayout->addWidget(label_18, 0, 0, 1, 1);

        edit_host = new QLineEdit(LoadLevelerConfigDialog);
        edit_host->setObjectName(QString::fromUtf8("edit_host"));

        gridLayout->addWidget(edit_host, 0, 2, 1, 1);

        spin_port = new QSpinBox(LoadLevelerConfigDialog);
        spin_port->setObjectName(QString::fromUtf8("spin_port"));
        spin_port->setMaximum(99999);
        spin_port->setValue(22);

        gridLayout->addWidget(spin_port, 0, 3, 1, 1);

        label_19 = new QLabel(LoadLevelerConfigDialog);
        label_19->setObjectName(QString::fromUtf8("label_19"));
        sizePolicy.setHeightForWidth(label_19->sizePolicy().hasHeightForWidth());
        label_19->setSizePolicy(sizePolicy);

        gridLayout->addWidget(label_19, 1, 0, 1, 1);

        edit_username = new QLineEdit(LoadLevelerConfigDialog);
        edit_username->setObjectName(QString::fromUtf8("edit_username"));

        gridLayout->addWidget(edit_username, 1, 2, 1, 2);

        label_20 = new QLabel(LoadLevelerConfigDialog);
        label_20->setObjectName(QString::fromUtf8("label_20"));
        sizePolicy.setHeightForWidth(label_20->sizePolicy().hasHeightForWidth());
        label_20->setSizePolicy(sizePolicy);

        gridLayout->addWidget(label_20, 2, 0, 1, 2);

        edit_rempath = new QLineEdit(LoadLevelerConfigDialog);
        edit_rempath->setObjectName(QString::fromUtf8("edit_rempath"));

        gridLayout->addWidget(edit_rempath, 2, 2, 1, 2);

        label_21 = new QLabel(LoadLevelerConfigDialog);
        label_21->setObjectName(QString::fromUtf8("label_21"));
        sizePolicy.setHeightForWidth(label_21->sizePolicy().hasHeightForWidth());
        label_21->setSizePolicy(sizePolicy);

        gridLayout->addWidget(label_21, 3, 0, 1, 1);

        edit_locpath = new QLineEdit(LoadLevelerConfigDialog);
        edit_locpath->setObjectName(QString::fromUtf8("edit_locpath"));

        gridLayout->addWidget(edit_locpath, 3, 2, 1, 2);

        label_launch = new QLabel(LoadLevelerConfigDialog);
        label_launch->setObjectName(QString::fromUtf8("label_launch"));
        sizePolicy.setHeightForWidth(label_launch->sizePolicy().hasHeightForWidth());
        label_launch->setSizePolicy(sizePolicy);

        gridLayout->addWidget(label_launch, 5, 0, 1, 1);

        edit_llsubmit = new QLineEdit(LoadLevelerConfigDialog);
        edit_llsubmit->setObjectName(QString::fromUtf8("edit_llsubmit"));

        gridLayout->addWidget(edit_llsubmit, 5, 2, 1, 2);

        label_qdel = new QLabel(LoadLevelerConfigDialog);
        label_qdel->setObjectName(QString::fromUtf8("label_qdel"));
        sizePolicy.setHeightForWidth(label_qdel->sizePolicy().hasHeightForWidth());
        label_qdel->setSizePolicy(sizePolicy);

        gridLayout->addWidget(label_qdel, 6, 0, 1, 1);

        label_check = new QLabel(LoadLevelerConfigDialog);
        label_check->setObjectName(QString::fromUtf8("label_check"));
        sizePolicy.setHeightForWidth(label_check->sizePolicy().hasHeightForWidth());
        label_check->setSizePolicy(sizePolicy);

        gridLayout->addWidget(label_check, 7, 0, 1, 1);

        edit_llcancel = new QLineEdit(LoadLevelerConfigDialog);
        edit_llcancel->setObjectName(QString::fromUtf8("edit_llcancel"));

        gridLayout->addWidget(edit_llcancel, 6, 2, 1, 2);

        edit_llq = new QLineEdit(LoadLevelerConfigDialog);
        edit_llq->setObjectName(QString::fromUtf8("edit_llq"));

        gridLayout->addWidget(edit_llq, 7, 2, 1, 2);

        label = new QLabel(LoadLevelerConfigDialog);
        label->setObjectName(QString::fromUtf8("label"));

        gridLayout->addWidget(label, 4, 0, 1, 1);

        edit_description = new QLineEdit(LoadLevelerConfigDialog);
        edit_description->setObjectName(QString::fromUtf8("edit_description"));

        gridLayout->addWidget(edit_description, 4, 2, 1, 2);

        buttonBox = new QDialogButtonBox(LoadLevelerConfigDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        gridLayout->addWidget(buttonBox, 12, 3, 1, 1);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(verticalSpacer, 10, 3, 1, 1);

        label_2 = new QLabel(LoadLevelerConfigDialog);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        gridLayout->addWidget(label_2, 8, 0, 1, 1);

        spin_interval = new QSpinBox(LoadLevelerConfigDialog);
        spin_interval->setObjectName(QString::fromUtf8("spin_interval"));
        spin_interval->setMinimum(1);
        spin_interval->setMaximum(99999);

        gridLayout->addWidget(spin_interval, 8, 2, 1, 2);

        cb_cleanRemoteOnStop = new QCheckBox(LoadLevelerConfigDialog);
        cb_cleanRemoteOnStop->setObjectName(QString::fromUtf8("cb_cleanRemoteOnStop"));

        gridLayout->addWidget(cb_cleanRemoteOnStop, 9, 0, 1, 4);

#ifndef QT_NO_SHORTCUT
        label_18->setBuddy(edit_host);
        label_19->setBuddy(edit_username);
        label_20->setBuddy(edit_rempath);
        label_21->setBuddy(edit_locpath);
        label_launch->setBuddy(edit_llsubmit);
        label_qdel->setBuddy(edit_llcancel);
        label_check->setBuddy(edit_llq);
        label->setBuddy(edit_description);
        label_2->setBuddy(spin_interval);
#endif // QT_NO_SHORTCUT
        QWidget::setTabOrder(edit_host, spin_port);
        QWidget::setTabOrder(spin_port, edit_username);
        QWidget::setTabOrder(edit_username, edit_rempath);
        QWidget::setTabOrder(edit_rempath, edit_locpath);
        QWidget::setTabOrder(edit_locpath, edit_description);
        QWidget::setTabOrder(edit_description, edit_llsubmit);
        QWidget::setTabOrder(edit_llsubmit, edit_llcancel);
        QWidget::setTabOrder(edit_llcancel, edit_llq);
        QWidget::setTabOrder(edit_llq, spin_interval);
        QWidget::setTabOrder(spin_interval, buttonBox);

        retranslateUi(LoadLevelerConfigDialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), LoadLevelerConfigDialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), LoadLevelerConfigDialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(LoadLevelerConfigDialog);
    } // setupUi

    void retranslateUi(QDialog *LoadLevelerConfigDialog)
    {
        LoadLevelerConfigDialog->setWindowTitle(QApplication::translate("LoadLevelerConfigDialog", "LoadLeveler Queue Configuration", 0, QApplication::UnicodeUTF8));
        label_18->setText(QApplication::translate("LoadLevelerConfigDialog", "Host:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        edit_host->setToolTip(QApplication::translate("LoadLevelerConfigDialog", "Address of host. Can use IP or host name.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        spin_port->setPrefix(QApplication::translate("LoadLevelerConfigDialog", "SSH Port ", 0, QApplication::UnicodeUTF8));
        label_19->setText(QApplication::translate("LoadLevelerConfigDialog", "User:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        edit_username->setToolTip(QApplication::translate("LoadLevelerConfigDialog", "Username on above host.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        label_20->setText(QApplication::translate("LoadLevelerConfigDialog", "Working directory (Server):", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        edit_rempath->setToolTip(QApplication::translate("LoadLevelerConfigDialog", "Path on remote host to use during optimizations. Do not use wildcard characters or BASH-specific characters (e.g. '~' in place of /home/user).", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        label_21->setText(QApplication::translate("LoadLevelerConfigDialog", "Working directory (Local):", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        edit_locpath->setToolTip(QApplication::translate("LoadLevelerConfigDialog", "Local path to store files", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        label_launch->setToolTip(QApplication::translate("LoadLevelerConfigDialog", "Command used to submit jobs to the PBS queue. Usually qsub.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        label_launch->setText(QApplication::translate("LoadLevelerConfigDialog", "Path to llsubmit:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        edit_llsubmit->setToolTip(QApplication::translate("LoadLevelerConfigDialog", "Command used to submit jobs to the PBS queue. Usually qsub.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        edit_llsubmit->setText(QApplication::translate("LoadLevelerConfigDialog", "llsubmit", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        label_qdel->setToolTip(QApplication::translate("LoadLevelerConfigDialog", "Command used to delete jobs from the queue. Usually qdel.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        label_qdel->setText(QApplication::translate("LoadLevelerConfigDialog", "Path to llcancel:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        label_check->setToolTip(QApplication::translate("LoadLevelerConfigDialog", "Command used to check the PBS queue. Use qstat.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        label_check->setText(QApplication::translate("LoadLevelerConfigDialog", "Path to llq:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        edit_llcancel->setToolTip(QApplication::translate("LoadLevelerConfigDialog", "Command used to delete jobs from the queue. Usually qdel.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        edit_llcancel->setText(QApplication::translate("LoadLevelerConfigDialog", "llcancel", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        edit_llq->setToolTip(QApplication::translate("LoadLevelerConfigDialog", "Command used to check the PBS queue. Use qstat.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        edit_llq->setText(QApplication::translate("LoadLevelerConfigDialog", "llq", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("LoadLevelerConfigDialog", "Description:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        edit_description->setToolTip(QApplication::translate("LoadLevelerConfigDialog", "Short description of optimization (used as %description% template keyword in input templates).", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        label_2->setText(QApplication::translate("LoadLevelerConfigDialog", "Queue refresh interval:", 0, QApplication::UnicodeUTF8));
        spin_interval->setSuffix(QApplication::translate("LoadLevelerConfigDialog", " sec", 0, QApplication::UnicodeUTF8));
        cb_cleanRemoteOnStop->setText(QApplication::translate("LoadLevelerConfigDialog", "Clean remote directories when finished", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class LoadLevelerConfigDialog: public Ui_LoadLevelerConfigDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_LOADLEVELERDIALOG_H
