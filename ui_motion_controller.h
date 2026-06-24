/********************************************************************************
** Form generated from reading UI file 'motion_controller.ui'
**
** Created by: Qt User Interface Compiler version 6.6.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MOTION_CONTROLLER_H
#define UI_MOTION_CONTROLLER_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "ptzcontrolwidget.h"

QT_BEGIN_NAMESPACE

class Ui_motion_controller
{
public:
    QVBoxLayout *verticalLayout_2;
    QTabWidget *control_widget;
    QWidget *tab;
    QHBoxLayout *horizontalLayout_2;
    PtzControlWidget *round_widget;
    QVBoxLayout *verticalLayout_5;
    QPushButton *pb_aim;
    QSpacerItem *verticalSpacer;
    QPushButton *pb_home;
    QWidget *tab_5;
    QVBoxLayout *verticalLayout;
    QGridLayout *gridLayout_7;
    QSpacerItem *horizontalSpacer_2;
    QPushButton *pb_move_y;
    QDoubleSpinBox *ds_move_y;
    QPushButton *pb_move_x;
    QDoubleSpinBox *ds_move_x;
    QPushButton *pb_all_axis_move;
    QWidget *tab_6;
    QVBoxLayout *verticalLayout_4;
    QGridLayout *gridLayout_8;
    QDoubleSpinBox *sb_y_speed;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer_3;
    QCheckBox *zoom_ratio;
    QSpacerItem *horizontalSpacer;
    QDoubleSpinBox *sb_x_speed_3;
    QDoubleSpinBox *sb_x_speed_2;
    QLabel *label_13;
    QLabel *label_12;
    QDoubleSpinBox *sb_x_speed;
    QFrame *line;
    QLabel *label_14;
    QLabel *label_16;
    QLabel *label_15;
    QSpacerItem *verticalSpacer_3;
    QHBoxLayout *horizontalLayout_3;
    QLabel *label_2;
    QLabel *l_mot_status;

    void setupUi(QWidget *motion_controller)
    {
        if (motion_controller->objectName().isEmpty())
            motion_controller->setObjectName("motion_controller");
        motion_controller->resize(235, 235);
        QSizePolicy sizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(motion_controller->sizePolicy().hasHeightForWidth());
        motion_controller->setSizePolicy(sizePolicy);
        motion_controller->setMinimumSize(QSize(235, 235));
        motion_controller->setMaximumSize(QSize(16777215, 250));
        motion_controller->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
        verticalLayout_2 = new QVBoxLayout(motion_controller);
        verticalLayout_2->setObjectName("verticalLayout_2");
        verticalLayout_2->setContentsMargins(0, 0, 0, 0);
        control_widget = new QTabWidget(motion_controller);
        control_widget->setObjectName("control_widget");
        control_widget->setEnabled(true);
        QSizePolicy sizePolicy1(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(control_widget->sizePolicy().hasHeightForWidth());
        control_widget->setSizePolicy(sizePolicy1);
        control_widget->setMinimumSize(QSize(0, 0));
        control_widget->setTabPosition(QTabWidget::TabPosition::North);
        control_widget->setTabShape(QTabWidget::TabShape::Rounded);
        control_widget->setElideMode(Qt::TextElideMode::ElideLeft);
        control_widget->setUsesScrollButtons(true);
        control_widget->setDocumentMode(false);
        control_widget->setTabsClosable(false);
        control_widget->setMovable(false);
        control_widget->setTabBarAutoHide(false);
        tab = new QWidget();
        tab->setObjectName("tab");
        horizontalLayout_2 = new QHBoxLayout(tab);
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        round_widget = new PtzControlWidget(tab);
        round_widget->setObjectName("round_widget");
        QSizePolicy sizePolicy2(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(round_widget->sizePolicy().hasHeightForWidth());
        round_widget->setSizePolicy(sizePolicy2);

        horizontalLayout_2->addWidget(round_widget);

        verticalLayout_5 = new QVBoxLayout();
        verticalLayout_5->setObjectName("verticalLayout_5");
        pb_aim = new QPushButton(tab);
        pb_aim->setObjectName("pb_aim");
        pb_aim->setEnabled(true);
        pb_aim->setMinimumSize(QSize(50, 50));
        pb_aim->setMaximumSize(QSize(50, 50));
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/img/image/arrows/aim.png"), QSize(), QIcon::Normal, QIcon::Off);
        pb_aim->setIcon(icon);
        pb_aim->setIconSize(QSize(48, 48));

        verticalLayout_5->addWidget(pb_aim);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout_5->addItem(verticalSpacer);

        pb_home = new QPushButton(tab);
        pb_home->setObjectName("pb_home");
        pb_home->setMinimumSize(QSize(50, 50));
        pb_home->setMaximumSize(QSize(50, 50));
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/img/image/arrows/home.png"), QSize(), QIcon::Normal, QIcon::Off);
        pb_home->setIcon(icon1);
        pb_home->setIconSize(QSize(48, 48));

        verticalLayout_5->addWidget(pb_home);


        horizontalLayout_2->addLayout(verticalLayout_5);

        control_widget->addTab(tab, QString());
        tab_5 = new QWidget();
        tab_5->setObjectName("tab_5");
        verticalLayout = new QVBoxLayout(tab_5);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setContentsMargins(6, 6, 6, 6);
        gridLayout_7 = new QGridLayout();
        gridLayout_7->setObjectName("gridLayout_7");
        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        gridLayout_7->addItem(horizontalSpacer_2, 2, 0, 1, 1);

        pb_move_y = new QPushButton(tab_5);
        pb_move_y->setObjectName("pb_move_y");

        gridLayout_7->addWidget(pb_move_y, 3, 1, 1, 1);

        ds_move_y = new QDoubleSpinBox(tab_5);
        ds_move_y->setObjectName("ds_move_y");
        ds_move_y->setFocusPolicy(Qt::FocusPolicy::ClickFocus);
        ds_move_y->setDecimals(3);
        ds_move_y->setMinimum(-90.000000000000000);
        ds_move_y->setMaximum(90.000000000000000);
        ds_move_y->setValue(0.000000000000000);

        gridLayout_7->addWidget(ds_move_y, 3, 0, 1, 1);

        pb_move_x = new QPushButton(tab_5);
        pb_move_x->setObjectName("pb_move_x");

        gridLayout_7->addWidget(pb_move_x, 1, 1, 1, 1);

        ds_move_x = new QDoubleSpinBox(tab_5);
        ds_move_x->setObjectName("ds_move_x");
        ds_move_x->setFocusPolicy(Qt::FocusPolicy::ClickFocus);
        ds_move_x->setDecimals(3);
        ds_move_x->setMinimum(-360.000000000000000);
        ds_move_x->setMaximum(360.000000000000000);
        ds_move_x->setValue(0.000000000000000);

        gridLayout_7->addWidget(ds_move_x, 1, 0, 1, 1);

        pb_all_axis_move = new QPushButton(tab_5);
        pb_all_axis_move->setObjectName("pb_all_axis_move");

        gridLayout_7->addWidget(pb_all_axis_move, 2, 1, 1, 1);


        verticalLayout->addLayout(gridLayout_7);

        control_widget->addTab(tab_5, QString());
        tab_6 = new QWidget();
        tab_6->setObjectName("tab_6");
        verticalLayout_4 = new QVBoxLayout(tab_6);
        verticalLayout_4->setObjectName("verticalLayout_4");
        verticalLayout_4->setContentsMargins(6, 6, 6, 6);
        gridLayout_8 = new QGridLayout();
        gridLayout_8->setSpacing(6);
        gridLayout_8->setObjectName("gridLayout_8");
        sb_y_speed = new QDoubleSpinBox(tab_6);
        sb_y_speed->setObjectName("sb_y_speed");
        QSizePolicy sizePolicy3(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Fixed);
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(sb_y_speed->sizePolicy().hasHeightForWidth());
        sb_y_speed->setSizePolicy(sizePolicy3);
        sb_y_speed->setMinimumSize(QSize(65, 0));
        sb_y_speed->setFocusPolicy(Qt::FocusPolicy::WheelFocus);
        sb_y_speed->setDecimals(3);
        sb_y_speed->setMinimum(0.001000000000000);
        sb_y_speed->setMaximum(300.000000000000000);
        sb_y_speed->setSingleStep(0.010000000000000);
        sb_y_speed->setValue(10.000000000000000);

        gridLayout_8->addWidget(sb_y_speed, 3, 1, 1, 1);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_3);

        zoom_ratio = new QCheckBox(tab_6);
        zoom_ratio->setObjectName("zoom_ratio");

        horizontalLayout->addWidget(zoom_ratio);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);


        gridLayout_8->addLayout(horizontalLayout, 1, 1, 1, 1);

        sb_x_speed_3 = new QDoubleSpinBox(tab_6);
        sb_x_speed_3->setObjectName("sb_x_speed_3");
        sb_x_speed_3->setEnabled(false);
        sizePolicy3.setHeightForWidth(sb_x_speed_3->sizePolicy().hasHeightForWidth());
        sb_x_speed_3->setSizePolicy(sizePolicy3);
        sb_x_speed_3->setMinimumSize(QSize(65, 0));
        sb_x_speed_3->setFocusPolicy(Qt::FocusPolicy::WheelFocus);
        sb_x_speed_3->setDecimals(3);
        sb_x_speed_3->setMinimum(0.001000000000000);
        sb_x_speed_3->setMaximum(300.000000000000000);
        sb_x_speed_3->setSingleStep(0.100000000000000);
        sb_x_speed_3->setValue(1.000000000000000);

        gridLayout_8->addWidget(sb_x_speed_3, 6, 1, 1, 1);

        sb_x_speed_2 = new QDoubleSpinBox(tab_6);
        sb_x_speed_2->setObjectName("sb_x_speed_2");
        sb_x_speed_2->setEnabled(false);
        sizePolicy3.setHeightForWidth(sb_x_speed_2->sizePolicy().hasHeightForWidth());
        sb_x_speed_2->setSizePolicy(sizePolicy3);
        sb_x_speed_2->setMinimumSize(QSize(65, 0));
        sb_x_speed_2->setFocusPolicy(Qt::FocusPolicy::WheelFocus);
        sb_x_speed_2->setDecimals(3);
        sb_x_speed_2->setMinimum(0.001000000000000);
        sb_x_speed_2->setMaximum(300.000000000000000);
        sb_x_speed_2->setSingleStep(0.100000000000000);
        sb_x_speed_2->setValue(1.000000000000000);

        gridLayout_8->addWidget(sb_x_speed_2, 5, 1, 1, 1);

        label_13 = new QLabel(tab_6);
        label_13->setObjectName("label_13");

        gridLayout_8->addWidget(label_13, 3, 0, 1, 1);

        label_12 = new QLabel(tab_6);
        label_12->setObjectName("label_12");

        gridLayout_8->addWidget(label_12, 2, 0, 1, 1);

        sb_x_speed = new QDoubleSpinBox(tab_6);
        sb_x_speed->setObjectName("sb_x_speed");
        sizePolicy3.setHeightForWidth(sb_x_speed->sizePolicy().hasHeightForWidth());
        sb_x_speed->setSizePolicy(sizePolicy3);
        sb_x_speed->setMinimumSize(QSize(65, 0));
        sb_x_speed->setFocusPolicy(Qt::FocusPolicy::WheelFocus);
        sb_x_speed->setDecimals(3);
        sb_x_speed->setMinimum(0.001000000000000);
        sb_x_speed->setMaximum(300.000000000000000);
        sb_x_speed->setSingleStep(0.100000000000000);
        sb_x_speed->setValue(10.000000000000000);

        gridLayout_8->addWidget(sb_x_speed, 2, 1, 1, 1);

        line = new QFrame(tab_6);
        line->setObjectName("line");
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);

        gridLayout_8->addWidget(line, 4, 0, 1, 2);

        label_14 = new QLabel(tab_6);
        label_14->setObjectName("label_14");

        gridLayout_8->addWidget(label_14, 5, 0, 1, 1);

        label_16 = new QLabel(tab_6);
        label_16->setObjectName("label_16");

        gridLayout_8->addWidget(label_16, 1, 0, 1, 1);

        label_15 = new QLabel(tab_6);
        label_15->setObjectName("label_15");

        gridLayout_8->addWidget(label_15, 6, 0, 1, 1);


        verticalLayout_4->addLayout(gridLayout_8);

        verticalSpacer_3 = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout_4->addItem(verticalSpacer_3);

        control_widget->addTab(tab_6, QString());

        verticalLayout_2->addWidget(control_widget);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        horizontalLayout_3->setContentsMargins(9, -1, 9, -1);
        label_2 = new QLabel(motion_controller);
        label_2->setObjectName("label_2");
        sizePolicy.setHeightForWidth(label_2->sizePolicy().hasHeightForWidth());
        label_2->setSizePolicy(sizePolicy);

        horizontalLayout_3->addWidget(label_2);

        l_mot_status = new QLabel(motion_controller);
        l_mot_status->setObjectName("l_mot_status");
        QSizePolicy sizePolicy4(QSizePolicy::Policy::MinimumExpanding, QSizePolicy::Policy::Preferred);
        sizePolicy4.setHorizontalStretch(0);
        sizePolicy4.setVerticalStretch(0);
        sizePolicy4.setHeightForWidth(l_mot_status->sizePolicy().hasHeightForWidth());
        l_mot_status->setSizePolicy(sizePolicy4);

        horizontalLayout_3->addWidget(l_mot_status);


        verticalLayout_2->addLayout(horizontalLayout_3);


        retranslateUi(motion_controller);

        control_widget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(motion_controller);
    } // setupUi

    void retranslateUi(QWidget *motion_controller)
    {
        motion_controller->setWindowTitle(QCoreApplication::translate("motion_controller", "Form", nullptr));
        pb_aim->setText(QString());
        pb_home->setText(QString());
        control_widget->setTabText(control_widget->indexOf(tab), QCoreApplication::translate("motion_controller", "\320\241\321\202\321\200\320\260\320\275\320\270\321\206\320\260", nullptr));
        pb_move_y->setText(QCoreApplication::translate("motion_controller", "Move to Y", nullptr));
        pb_move_x->setText(QCoreApplication::translate("motion_controller", "Move to X", nullptr));
        pb_all_axis_move->setText(QCoreApplication::translate("motion_controller", "Move to XY", nullptr));
        control_widget->setTabText(control_widget->indexOf(tab_5), QCoreApplication::translate("motion_controller", "Position", nullptr));
        zoom_ratio->setText(QString());
        label_13->setText(QCoreApplication::translate("motion_controller", "Velocity (Y), \302\260/sec", nullptr));
        label_12->setText(QCoreApplication::translate("motion_controller", "Velocity (X), \302\260/sec", nullptr));
        label_14->setText(QCoreApplication::translate("motion_controller", "Acceleration (X), \302\260/sec\302\262", nullptr));
        label_16->setText(QCoreApplication::translate("motion_controller", "Zoom ratio", nullptr));
        label_15->setText(QCoreApplication::translate("motion_controller", "Acceleration (Y),\302\260/sec\302\262", nullptr));
        control_widget->setTabText(control_widget->indexOf(tab_6), QCoreApplication::translate("motion_controller", "Settings", nullptr));
        label_2->setText(QCoreApplication::translate("motion_controller", "STATUS:", nullptr));
        l_mot_status->setText(QCoreApplication::translate("motion_controller", "Disconnected", nullptr));
    } // retranslateUi

};

namespace Ui {
    class motion_controller: public Ui_motion_controller {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MOTION_CONTROLLER_H
