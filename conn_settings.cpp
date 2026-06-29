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
    QStringList main;
    main.append(ui->url_main->text());
    main.append(ui->port_cam_RGB->text());
    main.append(ui->port_cam_IR->text());
    main.append(ui->port_meta->text());
    main.append(ui->port_meta_reciever->text());
    emit connect_to(main);
    this->close();
}
