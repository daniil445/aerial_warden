#include "conn_settings.h"
#include "ui_conn_settings.h"

conn_settings::conn_settings(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::conn_settings)
{
    ui->setupUi(this);
}

conn_settings::~conn_settings()
{
    delete ui;
}
void conn_settings::on_btn_save_clicked()
{
    QStringList cam1;
    cam1.append(ui->url_cam_RGB->text());
    cam1.append(ui->port_cam_RGB->text());
    QStringList cam2;
    cam2.append(ui->url_cam_IR->text());
    cam2.append(ui->port_cam_IR->text());
    QStringList main;
    main.append(ui->url_main->text());
    main.append(ui->port_meta->text());
    main.append(ui->port_meta_reciever->text());
    emit connect_to(cam1,cam2,main);
    this->close();
}
