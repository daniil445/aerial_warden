/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.6.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <videowidget.h>
#include "motion_controller/motion_controller.h"

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *actionSettings;
    QAction *actionInitialize_rotator;
    QWidget *centralwidget;
    QGridLayout *gridLayout;
    QTabWidget *tabWidget_2;
    QWidget *tab_3;
    QHBoxLayout *horizontalLayout_3;
    VideoWidget *openGLWidget_RGB;
    QWidget *tab_4;
    QHBoxLayout *horizontalLayout_4;
    VideoWidget *openGLWidget_IR;
    QGridLayout *gridLayout_3;
    QLabel *l_angle;
    QLabel *l_fps;
    QLabel *l_fps_4;
    QLabel *l_fps_3;
    QLabel *l_fps_5;
    QLabel *l_dist;
    QLabel *l_position;
    QLabel *l_text;
    QSpacerItem *horizontalSpacer;
    QGroupBox *groupBox_4;
    QVBoxLayout *verticalLayout_4;
    QSpacerItem *verticalSpacer;
    QSpacerItem *verticalSpacer_3;
    QTabWidget *tabWidget;
    QWidget *tab;
    QVBoxLayout *verticalLayout_2;
    QListWidget *obj_list;
    QWidget *tab_2;
    QVBoxLayout *verticalLayout_3;
    QListWidget *obj_list_move;
    QGroupBox *groupBox_2;
    QFormLayout *formLayout;
    QCheckBox *cb_show_moves;
    QCheckBox *cb_show_planes;
    QCheckBox *cb_show_birds;
    QCheckBox *cb_show_drones;
    QCheckBox *cb_show_mans;
    QCheckBox *cb_show_cars;
    QGroupBox *groupBox_5;
    QVBoxLayout *verticalLayout_5;
    QGridLayout *gridLayout_2;
    QPushButton *btn_follow;
    QCheckBox *alg_zoom;
    QPushButton *btn_search;
    QPushButton *btn_dist;
    QGroupBox *groupBox;
    QVBoxLayout *verticalLayout;
    QSpacerItem *verticalSpacer_2;
    QGroupBox *groupBox_3;
    QGridLayout *gridLayout_4;
    QLabel *label_4;
    QLabel *label_3;
    QHBoxLayout *horizontalLayout;
    QLabel *label;
    QLabel *l_zoom;
    QLabel *label_2;
    QSlider *zoom_slider;
    motion_controller *controls;
    QMenuBar *menubar;
    QMenu *menuMenu;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(1785, 882);
        MainWindow->setFocusPolicy(Qt::NoFocus);
        actionSettings = new QAction(MainWindow);
        actionSettings->setObjectName("actionSettings");
        actionInitialize_rotator = new QAction(MainWindow);
        actionInitialize_rotator->setObjectName("actionInitialize_rotator");
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        gridLayout = new QGridLayout(centralwidget);
        gridLayout->setObjectName("gridLayout");
        gridLayout->setContentsMargins(0, 0, 0, 0);
        tabWidget_2 = new QTabWidget(centralwidget);
        tabWidget_2->setObjectName("tabWidget_2");
        tab_3 = new QWidget();
        tab_3->setObjectName("tab_3");
        horizontalLayout_3 = new QHBoxLayout(tab_3);
        horizontalLayout_3->setSpacing(0);
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        horizontalLayout_3->setContentsMargins(0, 0, 0, 0);
        openGLWidget_RGB = new VideoWidget(tab_3);
        openGLWidget_RGB->setObjectName("openGLWidget_RGB");
        QSizePolicy sizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(openGLWidget_RGB->sizePolicy().hasHeightForWidth());
        openGLWidget_RGB->setSizePolicy(sizePolicy);
        openGLWidget_RGB->setMinimumSize(QSize(1280, 720));

        horizontalLayout_3->addWidget(openGLWidget_RGB);

        tabWidget_2->addTab(tab_3, QString());
        tab_4 = new QWidget();
        tab_4->setObjectName("tab_4");
        horizontalLayout_4 = new QHBoxLayout(tab_4);
        horizontalLayout_4->setSpacing(0);
        horizontalLayout_4->setObjectName("horizontalLayout_4");
        horizontalLayout_4->setContentsMargins(0, 0, 0, 0);
        openGLWidget_IR = new VideoWidget(tab_4);
        openGLWidget_IR->setObjectName("openGLWidget_IR");
        sizePolicy.setHeightForWidth(openGLWidget_IR->sizePolicy().hasHeightForWidth());
        openGLWidget_IR->setSizePolicy(sizePolicy);
        openGLWidget_IR->setMinimumSize(QSize(1270, 786));

        horizontalLayout_4->addWidget(openGLWidget_IR);

        tabWidget_2->addTab(tab_4, QString());

        gridLayout->addWidget(tabWidget_2, 0, 3, 1, 1);

        gridLayout_3 = new QGridLayout();
        gridLayout_3->setObjectName("gridLayout_3");
        l_angle = new QLabel(centralwidget);
        l_angle->setObjectName("l_angle");
        l_angle->setMaximumSize(QSize(16777215, 16777215));

        gridLayout_3->addWidget(l_angle, 0, 3, 1, 1);

        l_fps = new QLabel(centralwidget);
        l_fps->setObjectName("l_fps");

        gridLayout_3->addWidget(l_fps, 0, 1, 1, 1);

        l_fps_4 = new QLabel(centralwidget);
        l_fps_4->setObjectName("l_fps_4");
        QSizePolicy sizePolicy1(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Preferred);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(l_fps_4->sizePolicy().hasHeightForWidth());
        l_fps_4->setSizePolicy(sizePolicy1);
        l_fps_4->setMaximumSize(QSize(16777215, 16777215));

        gridLayout_3->addWidget(l_fps_4, 0, 2, 1, 1);

        l_fps_3 = new QLabel(centralwidget);
        l_fps_3->setObjectName("l_fps_3");
        sizePolicy1.setHeightForWidth(l_fps_3->sizePolicy().hasHeightForWidth());
        l_fps_3->setSizePolicy(sizePolicy1);
        l_fps_3->setMaximumSize(QSize(16777215, 16777215));

        gridLayout_3->addWidget(l_fps_3, 1, 2, 1, 1);

        l_fps_5 = new QLabel(centralwidget);
        l_fps_5->setObjectName("l_fps_5");
        sizePolicy1.setHeightForWidth(l_fps_5->sizePolicy().hasHeightForWidth());
        l_fps_5->setSizePolicy(sizePolicy1);
        l_fps_5->setMaximumSize(QSize(16777215, 16777215));

        gridLayout_3->addWidget(l_fps_5, 1, 0, 1, 1);

        l_dist = new QLabel(centralwidget);
        l_dist->setObjectName("l_dist");

        gridLayout_3->addWidget(l_dist, 1, 1, 1, 1);

        l_position = new QLabel(centralwidget);
        l_position->setObjectName("l_position");
        l_position->setMaximumSize(QSize(16777215, 16777215));

        gridLayout_3->addWidget(l_position, 1, 3, 1, 1);

        l_text = new QLabel(centralwidget);
        l_text->setObjectName("l_text");
        sizePolicy1.setHeightForWidth(l_text->sizePolicy().hasHeightForWidth());
        l_text->setSizePolicy(sizePolicy1);
        l_text->setMaximumSize(QSize(16777215, 16777215));

        gridLayout_3->addWidget(l_text, 0, 0, 1, 1);

        horizontalSpacer = new QSpacerItem(800, 20, QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Minimum);

        gridLayout_3->addItem(horizontalSpacer, 0, 4, 1, 1);


        gridLayout->addLayout(gridLayout_3, 1, 3, 1, 1);

        groupBox_4 = new QGroupBox(centralwidget);
        groupBox_4->setObjectName("groupBox_4");
        verticalLayout_4 = new QVBoxLayout(groupBox_4);
        verticalLayout_4->setObjectName("verticalLayout_4");
        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout_4->addItem(verticalSpacer);

        verticalSpacer_3 = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout_4->addItem(verticalSpacer_3);

        tabWidget = new QTabWidget(groupBox_4);
        tabWidget->setObjectName("tabWidget");
        tab = new QWidget();
        tab->setObjectName("tab");
        verticalLayout_2 = new QVBoxLayout(tab);
        verticalLayout_2->setObjectName("verticalLayout_2");
        obj_list = new QListWidget(tab);
        obj_list->setObjectName("obj_list");

        verticalLayout_2->addWidget(obj_list);

        tabWidget->addTab(tab, QString());
        tab_2 = new QWidget();
        tab_2->setObjectName("tab_2");
        verticalLayout_3 = new QVBoxLayout(tab_2);
        verticalLayout_3->setObjectName("verticalLayout_3");
        obj_list_move = new QListWidget(tab_2);
        obj_list_move->setObjectName("obj_list_move");

        verticalLayout_3->addWidget(obj_list_move);

        tabWidget->addTab(tab_2, QString());

        verticalLayout_4->addWidget(tabWidget);

        groupBox_2 = new QGroupBox(groupBox_4);
        groupBox_2->setObjectName("groupBox_2");
        formLayout = new QFormLayout(groupBox_2);
        formLayout->setObjectName("formLayout");
        formLayout->setContentsMargins(0, 0, 0, 0);
        cb_show_moves = new QCheckBox(groupBox_2);
        cb_show_moves->setObjectName("cb_show_moves");
        cb_show_moves->setChecked(false);

        formLayout->setWidget(1, QFormLayout::LabelRole, cb_show_moves);

        cb_show_planes = new QCheckBox(groupBox_2);
        cb_show_planes->setObjectName("cb_show_planes");
        cb_show_planes->setChecked(true);

        formLayout->setWidget(1, QFormLayout::FieldRole, cb_show_planes);

        cb_show_birds = new QCheckBox(groupBox_2);
        cb_show_birds->setObjectName("cb_show_birds");
        cb_show_birds->setChecked(true);

        formLayout->setWidget(2, QFormLayout::FieldRole, cb_show_birds);

        cb_show_drones = new QCheckBox(groupBox_2);
        cb_show_drones->setObjectName("cb_show_drones");
        cb_show_drones->setChecked(true);

        formLayout->setWidget(3, QFormLayout::FieldRole, cb_show_drones);

        cb_show_mans = new QCheckBox(groupBox_2);
        cb_show_mans->setObjectName("cb_show_mans");
        cb_show_mans->setChecked(true);

        formLayout->setWidget(4, QFormLayout::FieldRole, cb_show_mans);

        cb_show_cars = new QCheckBox(groupBox_2);
        cb_show_cars->setObjectName("cb_show_cars");
        cb_show_cars->setChecked(true);

        formLayout->setWidget(5, QFormLayout::FieldRole, cb_show_cars);


        verticalLayout_4->addWidget(groupBox_2);

        groupBox_5 = new QGroupBox(groupBox_4);
        groupBox_5->setObjectName("groupBox_5");
        verticalLayout_5 = new QVBoxLayout(groupBox_5);
        verticalLayout_5->setObjectName("verticalLayout_5");
        gridLayout_2 = new QGridLayout();
        gridLayout_2->setObjectName("gridLayout_2");
        btn_follow = new QPushButton(groupBox_5);
        btn_follow->setObjectName("btn_follow");
        btn_follow->setCheckable(true);

        gridLayout_2->addWidget(btn_follow, 0, 0, 1, 1);

        alg_zoom = new QCheckBox(groupBox_5);
        alg_zoom->setObjectName("alg_zoom");

        gridLayout_2->addWidget(alg_zoom, 0, 1, 1, 1);


        verticalLayout_5->addLayout(gridLayout_2);

        btn_search = new QPushButton(groupBox_5);
        btn_search->setObjectName("btn_search");
        btn_search->setCheckable(true);

        verticalLayout_5->addWidget(btn_search);

        btn_dist = new QPushButton(groupBox_5);
        btn_dist->setObjectName("btn_dist");
        btn_dist->setCheckable(true);

        verticalLayout_5->addWidget(btn_dist);


        verticalLayout_4->addWidget(groupBox_5);


        gridLayout->addWidget(groupBox_4, 0, 1, 2, 1);

        groupBox = new QGroupBox(centralwidget);
        groupBox->setObjectName("groupBox");
        groupBox->setMaximumSize(QSize(16777215, 16777215));
        verticalLayout = new QVBoxLayout(groupBox);
        verticalLayout->setObjectName("verticalLayout");
        verticalSpacer_2 = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout->addItem(verticalSpacer_2);

        groupBox_3 = new QGroupBox(groupBox);
        groupBox_3->setObjectName("groupBox_3");
        gridLayout_4 = new QGridLayout(groupBox_3);
        gridLayout_4->setObjectName("gridLayout_4");
        gridLayout_4->setContentsMargins(3, 3, 3, 3);
        label_4 = new QLabel(groupBox_3);
        label_4->setObjectName("label_4");
        label_4->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_4->addWidget(label_4, 1, 2, 1, 1);

        label_3 = new QLabel(groupBox_3);
        label_3->setObjectName("label_3");
        label_3->setAlignment(Qt::AlignCenter);

        gridLayout_4->addWidget(label_3, 1, 1, 1, 1);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(3);
        horizontalLayout->setObjectName("horizontalLayout");
        label = new QLabel(groupBox_3);
        label->setObjectName("label");
        label->setAlignment(Qt::AlignCenter);

        horizontalLayout->addWidget(label);

        l_zoom = new QLabel(groupBox_3);
        l_zoom->setObjectName("l_zoom");
        l_zoom->setMaximumSize(QSize(16777215, 16777215));
        l_zoom->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        horizontalLayout->addWidget(l_zoom);


        gridLayout_4->addLayout(horizontalLayout, 3, 1, 1, 1);

        label_2 = new QLabel(groupBox_3);
        label_2->setObjectName("label_2");

        gridLayout_4->addWidget(label_2, 1, 0, 1, 1);

        zoom_slider = new QSlider(groupBox_3);
        zoom_slider->setObjectName("zoom_slider");
        zoom_slider->setMinimum(1);
        zoom_slider->setMaximum(50);
        zoom_slider->setPageStep(5);
        zoom_slider->setOrientation(Qt::Horizontal);
        zoom_slider->setTickPosition(QSlider::TicksAbove);

        gridLayout_4->addWidget(zoom_slider, 2, 0, 1, 3);


        verticalLayout->addWidget(groupBox_3);

        controls = new motion_controller(groupBox);
        controls->setObjectName("controls");
        controls->setMinimumSize(QSize(235, 225));
        controls->setMaximumSize(QSize(235, 225));
        controls->setFocusPolicy(Qt::StrongFocus);

        verticalLayout->addWidget(controls);


        gridLayout->addWidget(groupBox, 0, 4, 2, 1);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 1785, 20));
        menuMenu = new QMenu(menubar);
        menuMenu->setObjectName("menuMenu");
        MainWindow->setMenuBar(menubar);

        menubar->addAction(menuMenu->menuAction());
        menuMenu->addAction(actionSettings);
        menuMenu->addAction(actionInitialize_rotator);

        retranslateUi(MainWindow);

        tabWidget_2->setCurrentIndex(0);
        tabWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        actionSettings->setText(QCoreApplication::translate("MainWindow", "Settings", nullptr));
        actionInitialize_rotator->setText(QCoreApplication::translate("MainWindow", "Initialize rotator", nullptr));
        tabWidget_2->setTabText(tabWidget_2->indexOf(tab_3), QCoreApplication::translate("MainWindow", "RGB", nullptr));
        tabWidget_2->setTabText(tabWidget_2->indexOf(tab_4), QCoreApplication::translate("MainWindow", "IR", nullptr));
        l_angle->setText(QString());
        l_fps->setText(QString());
        l_fps_4->setText(QCoreApplication::translate("MainWindow", "angle: ", nullptr));
        l_fps_3->setText(QCoreApplication::translate("MainWindow", "position: ", nullptr));
        l_fps_5->setText(QCoreApplication::translate("MainWindow", "distance: ", nullptr));
        l_dist->setText(QString());
        l_position->setText(QString());
        l_text->setText(QCoreApplication::translate("MainWindow", "frame count: ", nullptr));
        groupBox_4->setTitle(QCoreApplication::translate("MainWindow", "AI", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(tab), QCoreApplication::translate("MainWindow", "Identificated", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(tab_2), QCoreApplication::translate("MainWindow", "Detected", nullptr));
        groupBox_2->setTitle(QCoreApplication::translate("MainWindow", "Visibility", nullptr));
        cb_show_moves->setText(QCoreApplication::translate("MainWindow", "show_moves", nullptr));
        cb_show_planes->setText(QCoreApplication::translate("MainWindow", "show_planes", nullptr));
        cb_show_birds->setText(QCoreApplication::translate("MainWindow", "show_birds", nullptr));
        cb_show_drones->setText(QCoreApplication::translate("MainWindow", "show_drones", nullptr));
        cb_show_mans->setText(QCoreApplication::translate("MainWindow", "show_man", nullptr));
        cb_show_cars->setText(QCoreApplication::translate("MainWindow", "show_cars", nullptr));
        groupBox_5->setTitle(QCoreApplication::translate("MainWindow", "Algorythm", nullptr));
        btn_follow->setText(QCoreApplication::translate("MainWindow", "follow object", nullptr));
        alg_zoom->setText(QCoreApplication::translate("MainWindow", "with zoom", nullptr));
        btn_search->setText(QCoreApplication::translate("MainWindow", "search objects", nullptr));
        btn_dist->setText(QCoreApplication::translate("MainWindow", "calculate distance", nullptr));
        groupBox->setTitle(QCoreApplication::translate("MainWindow", "Sender", nullptr));
        groupBox_3->setTitle(QCoreApplication::translate("MainWindow", "Camera", nullptr));
        label_4->setText(QCoreApplication::translate("MainWindow", "50", nullptr));
        label_3->setText(QCoreApplication::translate("MainWindow", "25", nullptr));
        label->setText(QCoreApplication::translate("MainWindow", "Zoom:", nullptr));
        l_zoom->setText(QCoreApplication::translate("MainWindow", "1x", nullptr));
        label_2->setText(QCoreApplication::translate("MainWindow", "1", nullptr));
        menuMenu->setTitle(QCoreApplication::translate("MainWindow", "Menu", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
