/********************************************************************************
** Form generated from reading UI file 'pbsdialog.ui'
**
** Created by: Qt User Interface Compiler version 4.8.6
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_PBSDIALOG_H
#define UI_PBSDIALOG_H

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

class Ui_PbsConfigDialog
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
    QLineEdit *edit_qsub;
    QLabel *label_qdel;
    QLabel *label_check;
    QLineEdit *edit_qdel;
    QLineEdit *edit_qstat;
    QLabel *label;
    QLineEdit *edit_description;
    QDialogButtonBox *buttonBox;
    QSpacerItem *verticalSpacer;
    QLabel *label_2;
    QSpinBox *spin_interval;
    QCheckBox *cb_cleanRemoteOnStop;

    void setupUi(QDialog *PbsConfigDialog)
    {
        if (PbsConfigDialog->objectName().isEmpty())
            PbsConfigDialog->setObjectName(QString::fromUtf8("PbsConfigDialog"));
        PbsConfigDialog->resize(528, 271);
        QSizePolicy sizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(PbsConfigDialog->sizePolicy().hasHeightForWidth());
        PbsConfigDialog->setSizePolicy(sizePolicy);
        gridLayout = new QGridLayout(PbsConfigDialog);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        label_18 = new QLabel(PbsConfigDialog);
        label_18->setObjectName(QString::fromUtf8("label_18"));
        sizePolicy.setHeightForWidth(label_18->sizePolicy().hasHeightForWidth());
        label_18->setSizePolicy(sizePolicy);

        gridLayout->addWidget(label_18, 0, 0, 1, 1);

        edit_host = new QLineEdit(PbsConfigDialog);
        edit_host->setObjectName(QString::fromUtf8("edit_host"));

        gridLayout->addWidget(edit_host, 0, 2, 1, 1);

        spin_port = new QSpinBox(PbsConfigDialog);
        spin_port->setObjectName(QString::fromUtf8("spin_port"));
        spin_port->setMaximum(99999);
        spin_port->setValue(22);

        gridLayout->addWidget(spin_port, 0, 3, 1, 1);

        label_19 = new QLabel(PbsConfigDialog);
        label_19->setObjectName(QString::fromUtf8("label_19"));
        sizePolicy.setHeightForWidth(label_19->sizePolicy().hasHeightForWidth());
        label_19->setSizePolicy(sizePolicy);

        gridLayout->addWidget(label_19, 1, 0, 1, 1);

        edit_username = new QLineEdit(PbsConfigDialog);
        edit_username->setObjectName(QString::fromUtf8("edit_username"));

        gridLayout->addWidget(edit_username, 1, 2, 1, 2);

        label_20 = new QLabel(PbsConfigDialog);
        label_20->setObjectName(QString::fromUtf8("label_20"));
        sizePolicy.setHeightForWidth(label_20->sizePolicy().hasHeightForWidth());
        label_20->setSizePolicy(sizePolicy);

        gridLayout->addWidget(label_20, 2, 0, 1, 2);

        edit_rempath = new QLineEdit(PbsConfigDialog);
        edit_rempath->setObjectName(QString::fromUtf8("edit_rempath"));

        gridLayout->addWidget(edit_rempath, 2, 2, 1, 2);

        label_21 = new QLabel(PbsConfigDialog);
        label_21->setObjectName(QString::fromUtf8("label_21"));
        sizePolicy.setHeightForWidth(label_21->sizePolicy().hasHeightForWidth());
        label_21->setSizePolicy(sizePolicy);

        gridLayout->addWidget(label_21, 3, 0, 1, 1);

        edit_locpath = new QLineEdit(PbsConfigDialog);
        edit_locpath->setObjectName(QString::fromUtf8("edit_locpath"));

        gridLayout->addWidget(edit_locpath, 3, 2, 1, 2);

        label_launch = new QLabel(PbsConfigDialog);
        label_launch->setObjectName(QString::fromUtf8("label_launch"));
        sizePolicy.setHeightForWidth(label_launch->sizePolicy().hasHeightForWidth());
        label_launch->setSizePolicy(sizePolicy);

        gridLayout->addWidget(label_launch, 5, 0, 1, 1);

        edit_qsub = new QLineEdit(PbsConfigDialog);
        edit_qsub->setObjectName(QString::fromUtf8("edit_qsub"));

        gridLayout->addWidget(edit_qsub, 5, 2, 1, 2);

        label_qdel = new QLabel(PbsConfigDialog);
        label_qdel->setObjectName(QString::fromUtf8("label_qdel"));
        sizePolicy.setHeightForWidth(label_qdel->sizePolicy().hasHeightForWidth());
        label_qdel->setSizePolicy(sizePolicy);

        gridLayout->addWidget(label_qdel, 6, 0, 1, 1);

        label_check = new QLabel(PbsConfigDialog);
        label_check->setObjectName(QString::fromUtf8("label_check"));
        sizePolicy.setHeightForWidth(label_check->sizePolicy().hasHeightForWidth());
        label_check->setSizePolicy(sizePolicy);

        gridLayout->addWidget(label_check, 7, 0, 1, 1);

        edit_qdel = new QLineEdit(PbsConfigDialog);
        edit_qdel->setObjectName(QString::fromUtf8("edit_qdel"));

        gridLayout->addWidget(edit_qdel, 6, 2, 1, 2);

        edit_qstat = new QLineEdit(PbsConfigDialog);
        edit_qstat->setObjectName(QString::fromUtf8("edit_qstat"));

        gridLayout->addWidget(edit_qstat, 7, 2, 1, 2);

        label = new QLabel(PbsConfigDialog);
        label->setObjectName(QString::fromUtf8("label"));

        gridLayout->addWidget(label, 4, 0, 1, 1);

        edit_description = new QLineEdit(PbsConfigDialog);
        edit_description->setObjectName(QString::fromUtf8("edit_description"));

        gridLayout->addWidget(edit_description, 4, 2, 1, 2);

        buttonBox = new QDialogButtonBox(PbsConfigDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        gridLayout->addWidget(buttonBox, 12, 3, 1, 1);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(verticalSpacer, 10, 3, 1, 1);

        label_2 = new QLabel(PbsConfigDialog);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        gridLayout->addWidget(label_2, 8, 0, 1, 1);

        spin_interval = new QSpinBox(PbsConfigDialog);
        spin_interval->setObjectName(QString::fromUtf8("spin_interval"));
        spin_interval->setMinimum(1);
        spin_interval->setMaximum(99999);

        gridLayout->addWidget(spin_interval, 8, 2, 1, 2);

        cb_cleanRemoteOnStop = new QCheckBox(PbsConfigDialog);
        cb_cleanRemoteOnStop->setObjectName(QString::fromUtf8("cb_cleanRemoteOnStop"));

        gridLayout->addWidget(cb_cleanRemoteOnStop, 9, 0, 1, 4);

#ifndef QT_NO_SHORTCUT
        label_18->setBuddy(edit_host);
        label_19->setBuddy(edit_username);
        label_20->setBuddy(edit_rempath);
        label_21->setBuddy(edit_locpath);
        label_launch->setBuddy(edit_qsub);
        label_qdel->setBuddy(edit_qdel);
        label_check->setBuddy(edit_qstat);
        label->setBuddy(edit_description);
        label_2->setBuddy(spin_interval);
#endif // QT_NO_SHORTCUT
        QWidget::setTabOrder(edit_host, spin_port);
        QWidget::setTabOrder(spin_port, edit_username);
        QWidget::setTabOrder(edit_username, edit_rempath);
        QWidget::setTabOrder(edit_rempath, edit_locpath);
        QWidget::setTabOrder(edit_locpath, edit_description);
        QWidget::setTabOrder(edit_description, edit_qsub);
        QWidget::setTabOrder(edit_qsub, edit_qdel);
        QWidget::setTabOrder(edit_qdel, edit_qstat);
        QWidget::setTabOrder(edit_qstat, spin_interval);
        QWidget::setTabOrder(spin_interval, buttonBox);

        retranslateUi(PbsConfigDialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), PbsConfigDialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), PbsConfigDialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(PbsConfigDialog);
    } // setupUi

    void retranslateUi(QDialog *PbsConfigDialog)
    {
        PbsConfigDialog->setWindowTitle(QApplication::translate("PbsConfigDialog", "PBS Queue Configuration", 0, QApplication::UnicodeUTF8));
        label_18->setText(QApplication::translate("PbsConfigDialog", "Host:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        edit_host->setToolTip(QApplication::translate("PbsConfigDialog", "Address of host. Can use IP or host name.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        spin_port->setPrefix(QApplication::translate("PbsConfigDialog", "SSH Port ", 0, QApplication::UnicodeUTF8));
        label_19->setText(QApplication::translate("PbsConfigDialog", "User:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        edit_username->setToolTip(QApplication::translate("PbsConfigDialog", "Username on above host.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        label_20->setText(QApplication::translate("PbsConfigDialog", "Working directory (Server):", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        edit_rempath->setToolTip(QApplication::translate("PbsConfigDialog", "Path on remote host to use during optimizations. Do not use wildcard characters or BASH-specific characters (e.g. '~' in place of /home/user).", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        label_21->setText(QApplication::translate("PbsConfigDialog", "Working directory (Local):", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        edit_locpath->setToolTip(QApplication::translate("PbsConfigDialog", "Local path to store files", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        label_launch->setToolTip(QApplication::translate("PbsConfigDialog", "Command used to submit jobs to the PBS queue. Usually qsub.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        label_launch->setText(QApplication::translate("PbsConfigDialog", "Path to qsub:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        edit_qsub->setToolTip(QApplication::translate("PbsConfigDialog", "Command used to submit jobs to the PBS queue. Usually qsub.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        edit_qsub->setText(QApplication::translate("PbsConfigDialog", "qsub", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        label_qdel->setToolTip(QApplication::translate("PbsConfigDialog", "Command used to delete jobs from the queue. Usually qdel.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        label_qdel->setText(QApplication::translate("PbsConfigDialog", "Path to qdel:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        label_check->setToolTip(QApplication::translate("PbsConfigDialog", "Command used to check the PBS queue. Use qstat.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        label_check->setText(QApplication::translate("PbsConfigDialog", "Path to qstat:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        edit_qdel->setToolTip(QApplication::translate("PbsConfigDialog", "Command used to delete jobs from the queue. Usually qdel.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        edit_qdel->setText(QApplication::translate("PbsConfigDialog", "qdel", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        edit_qstat->setToolTip(QApplication::translate("PbsConfigDialog", "Command used to check the PBS queue. Use qstat.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        edit_qstat->setText(QApplication::translate("PbsConfigDialog", "qstat", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("PbsConfigDialog", "Description:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        edit_description->setToolTip(QApplication::translate("PbsConfigDialog", "Short description of optimization (used as %description% template keyword in input templates).", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        label_2->setText(QApplication::translate("PbsConfigDialog", "Queue refresh interval:", 0, QApplication::UnicodeUTF8));
        spin_interval->setSuffix(QApplication::translate("PbsConfigDialog", " sec", 0, QApplication::UnicodeUTF8));
        cb_cleanRemoteOnStop->setText(QApplication::translate("PbsConfigDialog", "Clean remote directories when finished", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class PbsConfigDialog: public Ui_PbsConfigDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PBSDIALOG_H
