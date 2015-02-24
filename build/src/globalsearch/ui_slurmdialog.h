/********************************************************************************
** Form generated from reading UI file 'slurmdialog.ui'
**
** Created by: Qt User Interface Compiler version 4.8.6
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SLURMDIALOG_H
#define UI_SLURMDIALOG_H

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

class Ui_SlurmConfigDialog
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
    QLineEdit *edit_sbatch;
    QLabel *label_scancel;
    QLabel *label_check;
    QLineEdit *edit_scancel;
    QLineEdit *edit_squeue;
    QLabel *label;
    QLineEdit *edit_description;
    QDialogButtonBox *buttonBox;
    QLabel *label_2;
    QSpacerItem *verticalSpacer;
    QSpinBox *spin_interval;
    QCheckBox *cb_cleanRemoteOnStop;

    void setupUi(QDialog *SlurmConfigDialog)
    {
        if (SlurmConfigDialog->objectName().isEmpty())
            SlurmConfigDialog->setObjectName(QString::fromUtf8("SlurmConfigDialog"));
        SlurmConfigDialog->resize(528, 271);
        QSizePolicy sizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(SlurmConfigDialog->sizePolicy().hasHeightForWidth());
        SlurmConfigDialog->setSizePolicy(sizePolicy);
        gridLayout = new QGridLayout(SlurmConfigDialog);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        label_18 = new QLabel(SlurmConfigDialog);
        label_18->setObjectName(QString::fromUtf8("label_18"));
        sizePolicy.setHeightForWidth(label_18->sizePolicy().hasHeightForWidth());
        label_18->setSizePolicy(sizePolicy);

        gridLayout->addWidget(label_18, 0, 0, 1, 1);

        edit_host = new QLineEdit(SlurmConfigDialog);
        edit_host->setObjectName(QString::fromUtf8("edit_host"));

        gridLayout->addWidget(edit_host, 0, 2, 1, 1);

        spin_port = new QSpinBox(SlurmConfigDialog);
        spin_port->setObjectName(QString::fromUtf8("spin_port"));
        spin_port->setMaximum(99999);
        spin_port->setValue(22);

        gridLayout->addWidget(spin_port, 0, 3, 1, 1);

        label_19 = new QLabel(SlurmConfigDialog);
        label_19->setObjectName(QString::fromUtf8("label_19"));
        sizePolicy.setHeightForWidth(label_19->sizePolicy().hasHeightForWidth());
        label_19->setSizePolicy(sizePolicy);

        gridLayout->addWidget(label_19, 1, 0, 1, 1);

        edit_username = new QLineEdit(SlurmConfigDialog);
        edit_username->setObjectName(QString::fromUtf8("edit_username"));

        gridLayout->addWidget(edit_username, 1, 2, 1, 2);

        label_20 = new QLabel(SlurmConfigDialog);
        label_20->setObjectName(QString::fromUtf8("label_20"));
        sizePolicy.setHeightForWidth(label_20->sizePolicy().hasHeightForWidth());
        label_20->setSizePolicy(sizePolicy);

        gridLayout->addWidget(label_20, 2, 0, 1, 2);

        edit_rempath = new QLineEdit(SlurmConfigDialog);
        edit_rempath->setObjectName(QString::fromUtf8("edit_rempath"));

        gridLayout->addWidget(edit_rempath, 2, 2, 1, 2);

        label_21 = new QLabel(SlurmConfigDialog);
        label_21->setObjectName(QString::fromUtf8("label_21"));
        sizePolicy.setHeightForWidth(label_21->sizePolicy().hasHeightForWidth());
        label_21->setSizePolicy(sizePolicy);

        gridLayout->addWidget(label_21, 3, 0, 1, 1);

        edit_locpath = new QLineEdit(SlurmConfigDialog);
        edit_locpath->setObjectName(QString::fromUtf8("edit_locpath"));

        gridLayout->addWidget(edit_locpath, 3, 2, 1, 2);

        label_launch = new QLabel(SlurmConfigDialog);
        label_launch->setObjectName(QString::fromUtf8("label_launch"));
        sizePolicy.setHeightForWidth(label_launch->sizePolicy().hasHeightForWidth());
        label_launch->setSizePolicy(sizePolicy);

        gridLayout->addWidget(label_launch, 5, 0, 1, 1);

        edit_sbatch = new QLineEdit(SlurmConfigDialog);
        edit_sbatch->setObjectName(QString::fromUtf8("edit_sbatch"));

        gridLayout->addWidget(edit_sbatch, 5, 2, 1, 2);

        label_scancel = new QLabel(SlurmConfigDialog);
        label_scancel->setObjectName(QString::fromUtf8("label_scancel"));
        sizePolicy.setHeightForWidth(label_scancel->sizePolicy().hasHeightForWidth());
        label_scancel->setSizePolicy(sizePolicy);

        gridLayout->addWidget(label_scancel, 6, 0, 1, 1);

        label_check = new QLabel(SlurmConfigDialog);
        label_check->setObjectName(QString::fromUtf8("label_check"));
        sizePolicy.setHeightForWidth(label_check->sizePolicy().hasHeightForWidth());
        label_check->setSizePolicy(sizePolicy);

        gridLayout->addWidget(label_check, 7, 0, 1, 1);

        edit_scancel = new QLineEdit(SlurmConfigDialog);
        edit_scancel->setObjectName(QString::fromUtf8("edit_scancel"));

        gridLayout->addWidget(edit_scancel, 6, 2, 1, 2);

        edit_squeue = new QLineEdit(SlurmConfigDialog);
        edit_squeue->setObjectName(QString::fromUtf8("edit_squeue"));

        gridLayout->addWidget(edit_squeue, 7, 2, 1, 2);

        label = new QLabel(SlurmConfigDialog);
        label->setObjectName(QString::fromUtf8("label"));

        gridLayout->addWidget(label, 4, 0, 1, 1);

        edit_description = new QLineEdit(SlurmConfigDialog);
        edit_description->setObjectName(QString::fromUtf8("edit_description"));

        gridLayout->addWidget(edit_description, 4, 2, 1, 2);

        buttonBox = new QDialogButtonBox(SlurmConfigDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        gridLayout->addWidget(buttonBox, 11, 3, 1, 1);

        label_2 = new QLabel(SlurmConfigDialog);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        gridLayout->addWidget(label_2, 8, 0, 1, 1);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(verticalSpacer, 10, 3, 1, 1);

        spin_interval = new QSpinBox(SlurmConfigDialog);
        spin_interval->setObjectName(QString::fromUtf8("spin_interval"));
        spin_interval->setMinimum(1);
        spin_interval->setMaximum(99999);

        gridLayout->addWidget(spin_interval, 8, 2, 1, 2);

        cb_cleanRemoteOnStop = new QCheckBox(SlurmConfigDialog);
        cb_cleanRemoteOnStop->setObjectName(QString::fromUtf8("cb_cleanRemoteOnStop"));

        gridLayout->addWidget(cb_cleanRemoteOnStop, 9, 0, 1, 4);

#ifndef QT_NO_SHORTCUT
        label_18->setBuddy(edit_host);
        label_19->setBuddy(edit_username);
        label_20->setBuddy(edit_rempath);
        label_21->setBuddy(edit_locpath);
        label_launch->setBuddy(edit_sbatch);
        label_scancel->setBuddy(edit_scancel);
        label_check->setBuddy(edit_squeue);
        label->setBuddy(edit_description);
#endif // QT_NO_SHORTCUT
        QWidget::setTabOrder(edit_host, spin_port);
        QWidget::setTabOrder(spin_port, edit_username);
        QWidget::setTabOrder(edit_username, edit_rempath);
        QWidget::setTabOrder(edit_rempath, edit_locpath);
        QWidget::setTabOrder(edit_locpath, edit_description);
        QWidget::setTabOrder(edit_description, edit_sbatch);
        QWidget::setTabOrder(edit_sbatch, edit_scancel);
        QWidget::setTabOrder(edit_scancel, edit_squeue);
        QWidget::setTabOrder(edit_squeue, spin_interval);
        QWidget::setTabOrder(spin_interval, cb_cleanRemoteOnStop);
        QWidget::setTabOrder(cb_cleanRemoteOnStop, buttonBox);

        retranslateUi(SlurmConfigDialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), SlurmConfigDialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), SlurmConfigDialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(SlurmConfigDialog);
    } // setupUi

    void retranslateUi(QDialog *SlurmConfigDialog)
    {
        SlurmConfigDialog->setWindowTitle(QApplication::translate("SlurmConfigDialog", "SLURM Queue Configuration", 0, QApplication::UnicodeUTF8));
        label_18->setText(QApplication::translate("SlurmConfigDialog", "Host:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        edit_host->setToolTip(QApplication::translate("SlurmConfigDialog", "Address of host. Can use IP or host name.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        spin_port->setPrefix(QApplication::translate("SlurmConfigDialog", "SSH Port ", 0, QApplication::UnicodeUTF8));
        label_19->setText(QApplication::translate("SlurmConfigDialog", "User:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        edit_username->setToolTip(QApplication::translate("SlurmConfigDialog", "Username on above host.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        label_20->setText(QApplication::translate("SlurmConfigDialog", "Working directory (Server):", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        edit_rempath->setToolTip(QApplication::translate("SlurmConfigDialog", "Path on remote host to use during optimizations. Do not use wildcard characters or BASH-specific characters (e.g. '~' in place of /home/user).", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        label_21->setText(QApplication::translate("SlurmConfigDialog", "Working directory (Local):", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        edit_locpath->setToolTip(QApplication::translate("SlurmConfigDialog", "Local path to store files", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        label_launch->setToolTip(QApplication::translate("SlurmConfigDialog", "Command used to submit jobs to the SLURM queue. Usually sbatch.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        label_launch->setText(QApplication::translate("SlurmConfigDialog", "Path to sbatch:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        edit_sbatch->setToolTip(QApplication::translate("SlurmConfigDialog", "Command used to submit jobs to the SLURM queue. Usually sbatch.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        edit_sbatch->setText(QApplication::translate("SlurmConfigDialog", "sbatch", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        label_scancel->setToolTip(QApplication::translate("SlurmConfigDialog", "Command used to delete jobs from the queue. Usually scancel.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        label_scancel->setText(QApplication::translate("SlurmConfigDialog", "Path to scancel:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        label_check->setToolTip(QApplication::translate("SlurmConfigDialog", "Command used to check the SLURM queue. Usually squeue.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        label_check->setText(QApplication::translate("SlurmConfigDialog", "Path to squeue:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        edit_scancel->setToolTip(QApplication::translate("SlurmConfigDialog", "Command used to delete jobs from the queue. Usually scancel.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        edit_scancel->setText(QApplication::translate("SlurmConfigDialog", "scancel", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        edit_squeue->setToolTip(QApplication::translate("SlurmConfigDialog", "Command used to check the SLURM queue. Use squeue.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        edit_squeue->setText(QApplication::translate("SlurmConfigDialog", "squeue", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("SlurmConfigDialog", "Description:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        edit_description->setToolTip(QApplication::translate("SlurmConfigDialog", "Short description of optimization (used as %description% template keyword in input templates).", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        label_2->setText(QApplication::translate("SlurmConfigDialog", "Queue refresh interval:", 0, QApplication::UnicodeUTF8));
        cb_cleanRemoteOnStop->setText(QApplication::translate("SlurmConfigDialog", "Clean remote directories when finished", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class SlurmConfigDialog: public Ui_SlurmConfigDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SLURMDIALOG_H
