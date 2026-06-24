/********************************************************************************
** Form generated from reading UI file 'conn_settings.ui'
**
** Created by: Qt User Interface Compiler version 6.6.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CONN_SETTINGS_H
#define UI_CONN_SETTINGS_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>

QT_BEGIN_NAMESPACE

class Ui_conn_settings
{
public:
    QGridLayout *gridLayout;
    QLabel *label_2;
    QLineEdit *url_cam_RGB;
    QLineEdit *port_meta;
    QLineEdit *url_cam_IR;
    QLabel *label_11;
    QLabel *label_9;
    QLabel *label_5;
    QLabel *label_4;
    QLineEdit *port_cam_RGB;
    QLineEdit *port_meta_reciever;
    QLabel *label;
    QLabel *label_7;
    QLabel *label_3;
    QLineEdit *port_cam_IR;
    QLabel *label_10;
    QLabel *label_6;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QPushButton *btn_save;
    QLineEdit *url_main;
    QFrame *line;

    void setupUi(QDialog *conn_settings)
    {
        if (conn_settings->objectName().isEmpty())
            conn_settings->setObjectName("conn_settings");
        conn_settings->resize(400, 162);
        gridLayout = new QGridLayout(conn_settings);
        gridLayout->setObjectName("gridLayout");
        label_2 = new QLabel(conn_settings);
        label_2->setObjectName("label_2");

        gridLayout->addWidget(label_2, 0, 0, 1, 1);

        url_cam_RGB = new QLineEdit(conn_settings);
        url_cam_RGB->setObjectName("url_cam_RGB");

        gridLayout->addWidget(url_cam_RGB, 0, 2, 1, 1);

        port_meta = new QLineEdit(conn_settings);
        port_meta->setObjectName("port_meta");
        port_meta->setMaximumSize(QSize(40, 16777215));
        port_meta->setMaxLength(5);

        gridLayout->addWidget(port_meta, 3, 4, 1, 1);

        url_cam_IR = new QLineEdit(conn_settings);
        url_cam_IR->setObjectName("url_cam_IR");

        gridLayout->addWidget(url_cam_IR, 1, 2, 1, 1);

        label_11 = new QLabel(conn_settings);
        label_11->setObjectName("label_11");

        gridLayout->addWidget(label_11, 1, 5, 1, 1);

        label_9 = new QLabel(conn_settings);
        label_9->setObjectName("label_9");

        gridLayout->addWidget(label_9, 1, 3, 1, 1);

        label_5 = new QLabel(conn_settings);
        label_5->setObjectName("label_5");

        gridLayout->addWidget(label_5, 4, 0, 1, 1);

        label_4 = new QLabel(conn_settings);
        label_4->setObjectName("label_4");

        gridLayout->addWidget(label_4, 0, 3, 1, 1);

        port_cam_RGB = new QLineEdit(conn_settings);
        port_cam_RGB->setObjectName("port_cam_RGB");
        port_cam_RGB->setMaximumSize(QSize(40, 16777215));
        port_cam_RGB->setMaxLength(5);

        gridLayout->addWidget(port_cam_RGB, 0, 4, 1, 1);

        port_meta_reciever = new QLineEdit(conn_settings);
        port_meta_reciever->setObjectName("port_meta_reciever");
        port_meta_reciever->setMaximumSize(QSize(40, 16777215));
        port_meta_reciever->setMaxLength(5);

        gridLayout->addWidget(port_meta_reciever, 4, 4, 1, 1);

        label = new QLabel(conn_settings);
        label->setObjectName("label");

        gridLayout->addWidget(label, 0, 1, 1, 1);

        label_7 = new QLabel(conn_settings);
        label_7->setObjectName("label_7");

        gridLayout->addWidget(label_7, 1, 1, 1, 1);

        label_3 = new QLabel(conn_settings);
        label_3->setObjectName("label_3");

        gridLayout->addWidget(label_3, 3, 0, 1, 1);

        port_cam_IR = new QLineEdit(conn_settings);
        port_cam_IR->setObjectName("port_cam_IR");
        port_cam_IR->setMaximumSize(QSize(40, 16777215));
        port_cam_IR->setMaxLength(5);

        gridLayout->addWidget(port_cam_IR, 1, 4, 1, 1);

        label_10 = new QLabel(conn_settings);
        label_10->setObjectName("label_10");

        gridLayout->addWidget(label_10, 0, 5, 1, 1);

        label_6 = new QLabel(conn_settings);
        label_6->setObjectName("label_6");

        gridLayout->addWidget(label_6, 3, 3, 1, 1);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        btn_save = new QPushButton(conn_settings);
        btn_save->setObjectName("btn_save");

        horizontalLayout->addWidget(btn_save);


        gridLayout->addLayout(horizontalLayout, 5, 0, 1, 6);

        url_main = new QLineEdit(conn_settings);
        url_main->setObjectName("url_main");

        gridLayout->addWidget(url_main, 3, 2, 1, 1);

        line = new QFrame(conn_settings);
        line->setObjectName("line");
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);

        gridLayout->addWidget(line, 2, 0, 1, 6);


        retranslateUi(conn_settings);

        QMetaObject::connectSlotsByName(conn_settings);
    } // setupUi

    void retranslateUi(QDialog *conn_settings)
    {
        conn_settings->setWindowTitle(QCoreApplication::translate("conn_settings", "Dialog", nullptr));
        label_2->setText(QCoreApplication::translate("conn_settings", "RTSP", nullptr));
        url_cam_RGB->setText(QCoreApplication::translate("conn_settings", "192.168.26.108", nullptr));
        port_meta->setText(QCoreApplication::translate("conn_settings", "5010", nullptr));
        url_cam_IR->setText(QCoreApplication::translate("conn_settings", "192.168.26.176", nullptr));
        label_11->setText(QCoreApplication::translate("conn_settings", "IR", nullptr));
        label_9->setText(QCoreApplication::translate("conn_settings", ":", nullptr));
        label_5->setText(QCoreApplication::translate("conn_settings", "Meta", nullptr));
        label_4->setText(QCoreApplication::translate("conn_settings", ":", nullptr));
        port_cam_RGB->setText(QCoreApplication::translate("conn_settings", "554", nullptr));
        port_meta_reciever->setText(QCoreApplication::translate("conn_settings", "5013", nullptr));
        label->setText(QCoreApplication::translate("conn_settings", "rtsp://", nullptr));
        label_7->setText(QCoreApplication::translate("conn_settings", "rtsp://", nullptr));
        label_3->setText(QCoreApplication::translate("conn_settings", "Main", nullptr));
        port_cam_IR->setText(QCoreApplication::translate("conn_settings", "5540", nullptr));
        label_10->setText(QCoreApplication::translate("conn_settings", "RGB", nullptr));
        label_6->setText(QCoreApplication::translate("conn_settings", ":", nullptr));
        btn_save->setText(QCoreApplication::translate("conn_settings", "Connect", nullptr));
        url_main->setText(QCoreApplication::translate("conn_settings", "192.168.26.176", nullptr));
    } // retranslateUi

};

namespace Ui {
    class conn_settings: public Ui_conn_settings {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CONN_SETTINGS_H
