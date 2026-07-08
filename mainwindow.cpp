#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QJsonArray>
#include <QSettings>
#include <RtspReceiver.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //// VIdeo
    connect(ui->cb_show_moves, &QCheckBox::toggled, [=](bool val) {ui->openGLWidget_RGB->motion_visible=val;});
    connect(ui->cb_show_planes, &QCheckBox::toggled,[=](bool val) {ui->openGLWidget_RGB->motion_planes=val;});
    connect(ui->cb_show_drones, &QCheckBox::toggled,[=](bool val) {ui->openGLWidget_RGB->motion_drones=val;});
    connect(ui->cb_show_birds, &QCheckBox::toggled, [=](bool val) {ui->openGLWidget_RGB->motion_birds=val;});
    connect(ui->cb_show_cars, &QCheckBox::toggled,  [=](bool val) {ui->openGLWidget_RGB->motion_cars=val;});
    connect(ui->cb_show_mans, &QCheckBox::toggled,  [=](bool val) {ui->openGLWidget_RGB->motion_mans=val;});
    connect(ui->cb_show_aim, &QCheckBox::toggled,   [=](bool val) {ui->openGLWidget_RGB->show_aim=val;});
    connect(ui->cb_show_degree, &QCheckBox::toggled,[=](bool val) {ui->openGLWidget_RGB->show_degree=val;});
    connect(ui->cb_show_text, &QCheckBox::toggled,  [=](bool val) {ui->openGLWidget_RGB->show_text=val;});

    dialog_sett= new conn_settings;
    connect(dialog_sett, &conn_settings::connect_to, this, &MainWindow::try_to_connect);
    connect(ui->actionSettings, &QAction::triggered, dialog_sett, &QWidget::show);
    connect(ui->actionInitialize_rotator, &QAction::triggered,[=]() {sender->sendCmd("init");});

    rtcp_receiver_RGB = new RtspReceiver(this);
    connect(rtcp_receiver_RGB, &RtspReceiver::frameReady, ui->openGLWidget_RGB, &VideoWidget::setFrame,Qt::QueuedConnection);

    rtcp_receiver_IR = new RtspReceiver(this);
    connect(rtcp_receiver_IR, &RtspReceiver::frameReady, ui->openGLWidget_IR, &VideoWidget::setFrame);


    sender = new CommandSender(this);

//    connect(ui->zoomtest, &QDoubleSpinBox::valueChanged, [=](double var) {sender->sendZoom(main_stream,var);});
//    connect(ui->zoom_slider, &QSlider::sliderReleased, [=]() {sender->sendZoom(main_stream,zoomT[qBound(1, ui->zoom_slider->value(), 50) - 1]);});
    connect(ui->zoom_slider, &QSlider::valueChanged, [=](int move) {// int move=ui->zoom_slider->value();
                                                               ui->zoom_info->setText(QString::number(move,'d',1)+"x");
                                                               sender->sendZoom(main_stream, move);
    });

    connect(ui->controls, &motion_controller::send_zoom,ui->zoom_slider,&QSlider::setValue);

    recorder = new VideoRecorder(this);
    // connect(ui->openGLWidget_RGB, &VideoWidget::frameForRecord, recorder, &VideoRecorder::addFrame);
    follower= new target_escort(this);
    //// linkers
    follower->link_storages(&storage_move,&storage,sender);
    ui->openGLWidget_RGB->link_storages(&storage_move,&storage);
    follower->start();

    connect(ui->alg_zoom, &QCheckBox::toggled, [=](bool var) {follower->follow_zoom=var;});
    connect(ui->openGLWidget_RGB, &VideoWidget::update_size,follower, &target_escort::update_size);
    connect(follower, &target_escort::move_by_object, sender, &CommandSender::sendMove);
    connect(follower,&target_escort::move_to_object,this,&MainWindow::sendMoveToCommand);
    connect(this, &MainWindow::update_focus,follower, &target_escort::update_focus);
    connect(this, &MainWindow::update_focus,ui->openGLWidget_RGB, &VideoWidget::update_focus);
    connect(ui->openGLWidget_RGB,&VideoWidget::update_list,this, &MainWindow::update_list);
    connect(follower, &target_escort::zoom_to_object, [=](double val) {sender->sendZoom(main_stream,val);});


    connect(ui->controls, &motion_controller::moveCommand,this,&MainWindow::sendMoveCommand);
    connect(ui->controls, &motion_controller::moveToCommand,this,&MainWindow::sendMoveToCommand);

    connect(ui->controls, &motion_controller::update_speed_x_y,follower,&target_escort::update_speed_x_y);
    connect(ui->controls, &motion_controller::change_cam,ui->widget_cam,&QTabWidget::setCurrentIndex);
            // [=](int val) { qDebug()<<"val"<<val;ui->widget_cam->setCurrentIndex(val);});

    UDP_receiver = new CommandReceiver(this);
    connect(UDP_receiver, &CommandReceiver::metaObjectsReceived,ui->openGLWidget_RGB, &VideoWidget::setMeta);
    connect(ui->openGLWidget_RGB, &VideoWidget::set_meta_f_z,this,&MainWindow::update_meta);
    connect(ui->openGLWidget_RGB, &VideoWidget::set_meta_f_z,follower,&target_escort::update_meta);
    connect(this, &MainWindow::update_zoom,ui->controls, &motion_controller::update_zoom);
    connect(ui->openGLWidget_RGB, &VideoWidget::set_meta_a_p,this,&MainWindow::update_meta_pos);
    connect(ui->openGLWidget_RGB, &VideoWidget::set_meta_a_p,follower,&target_escort::update_meta_pos);
    connect(ui->openGLWidget_RGB, &VideoWidget::set_meta_a_p,ui->controls, &motion_controller::update_aim);
    connect(ui->openGLWidget_RGB, &VideoWidget::set_meta_d,this,&MainWindow::update_distance);
    connect(ui->openGLWidget_RGB, &VideoWidget::moveToCommand,this,&MainWindow::sendMoveToCommandPos);


    QSettings settings("config.ini", QSettings::IniFormat);
    QStringList name= {"Plane","Bird","Drone","Human","Car"};
    QVariantList perfect_size= {78,114,26,36,56};
    settings.setValue("obj_name" ,name);
    settings.setValue("obj_size" ,perfect_size);

//    qputenv("GST_DEBUG", "4");
//    qputenv("GST_PLUGIN_PATH", (QCoreApplication::applicationDirPath() + "/gstreamer-1.0").toUtf8());
//    qputenv("GST_PLUGIN_SYSTEM_PATH", (QCoreApplication::applicationDirPath() + "/gstreamer-1.0").toUtf8());
//    qApp->installEventFilter(this);

//         auto base = QCoreApplication::applicationDirPath();//for release

//         qputenv("PATH", (base + ";" + base + "/gstreamer-1.0").toUtf8());

//         qputenv("GST_PLUGIN_PATH", (base + "/gstreamer-1.0").toUtf8());
//         qputenv("GST_PLUGIN_SYSTEM_PATH", (base + "/gstreamer-1.0").toUtf8());
//         qputenv("GST_REGISTRY_FORK", "no"); // важно для Windows debug
//         gst_init(nullptr, nullptr);

}

MainWindow::~MainWindow()
{
    follower->stop();
    follower->wait();
    delete ui;
}

void MainWindow::try_to_connect(QStringList url_main)
{
    main_ip=url_main[0];
    main_port=url_main[1].toInt();
    meta_port=url_main[2].toInt();
    rtcp_receiver_RGB->setUrl("rtsp://"+main_ip+":"+url_main[3]+"/rgb");
    rtcp_receiver_RGB->start();
//    rtcp_receiver_IR ->setUrl("rtsp://"+main_ip+":"+url_main[4]+"/ir");
//    rtcp_receiver_IR->start();

    qDebug()<<"update stream"<<"rtsp://"<<main_ip+":"+url_main[3];
    qDebug()<<"update stream"<<"rtsp://"<<main_ip+":"+url_main[4];
    qDebug()<<"update meta"<<"UDP"<<main_ip<<url_main[1]<<url_main[2];

    UDP_receiver->bind(main_ip,meta_port);
    sender->sendAddress(main_ip,main_port);
    sender->sendIp();
}

void MainWindow::update_meta(quint64 frame, int zoom)
{
    ui->l_fps->setText(QString::number(frame/1000000000.0,'d',3));
//    qDebug()<<"ui zoom"<<zoom<<ui->zoom_slider->value()<<abs(zoom -ui->zoom_slider->value());
    if(!ui->zoom_slider->isEnabled()){
        ui->zoom_slider->setEnabled(true);
        ui->zoom_slider->setValue((int)zoom);
    }
    update_zoom(zoom);
    ui->widget_cam_zoom->setEnabled(zoom!=-1);
    ui->actionInitialize_rotator->setEnabled(zoom!=-1);
    emit update_zoom(zoom);
}

void MainWindow::update_meta_pos(QVector2D ang, QVector3D pos)
{

    ui->l_angle->setText(QString("a = %0 b = %1").arg(ang.x()).arg(ang.y()));
    ui->l_position->setText((QString("x = %0 y = %1 z = %2").arg(pos.x()).arg(pos.y()).arg(pos.z())));
}

void MainWindow::update_distance(double dist)
{
    ui->l_dist->setText(QString::number(dist,'d',3)+" m");
}

void MainWindow::sendMoveCommand(const QString& cmd, double speed, bool pressed)
{
    qDebug()<<"sendMove"<<cmd<<speed<<pressed;
    if(cmd=="stop"){
        sender->sendCmd(cmd);
    }else if(cmd=="home"){
        sender->sendCmd(QString("%0|%1").arg(speed).arg(speed));
    }else if(cmd=="aim"){
        sender->sendCmd(cmd);
    }else{
        QString temp;
        if(!pressed)speed=0;
        temp_speed_x=cmd=="right"?speed:(cmd=="left"?-speed:temp_speed_x);
        temp_speed_y=cmd=="down"?speed:(cmd=="up"?-speed:temp_speed_y);
        temp= QString("%0|%1").arg(temp_speed_x).arg(temp_speed_y);
        sender->sendMove(temp);
//        qDebug()<<"sendMove"<<temp<<pressed<<speed<<temp_speed_x<<temp_speed_y;
    }
}
void MainWindow::sendMoveToCommandPos(QPointF pos)
{
    sendMoveToCommand(pos.x(),pos.y(),5,5);
}

void MainWindow::sendMoveToCommand(double pos_x, double pos_y, double speed_x, double speed_y)
{
    if(speed_x==0 && speed_y==0 ) sender->sendCmd("stop");
    else sender->sendMoveTo(QString("%0|%1|%2|%3").arg(pos_x).arg(-pos_y).arg(speed_x).arg(speed_y));
}

void MainWindow::update_list()
{
    foreach(QString key, storage_move.keys()){
        QList<QListWidgetItem*> items = ui->obj_list_move->findItems(key, Qt::MatchExactly);
        if (items.isEmpty())  ui->obj_list_move->addItem(key);
    }

    foreach(QString key, storage.keys()){
        QList<QListWidgetItem*> items = ui->obj_list->findItems(key, Qt::MatchExactly);
        if (items.isEmpty())  ui->obj_list->addItem(key);
    }


    for (int i = ui->obj_list_move->count() - 1; i >= 0; --i)
    {
        QListWidgetItem* item = ui->obj_list_move->item(i);
        if(!storage_move.contains(item->text())) delete ui->obj_list_move->takeItem(i);
    }

    for (int i = ui->obj_list->count() - 1; i >= 0; --i)
    {
        QListWidgetItem* item = ui->obj_list->item(i);
        if(!storage.contains(item->text())) delete ui->obj_list->takeItem(i);
    }

    if(ui->obj_list->findItems(focus_name, Qt::MatchExactly).length()==0){
//        ui->btn_follow->setEnabled(false);
//        ui->btn_follow->clicked(false);
//        on_btn_follow_clicked(false);
    }
}


void MainWindow::on_obj_list_itemClicked(QListWidgetItem *item)
{
    focus_name=item->text();
    ui->btn_follow->setEnabled(true);
    if(storage.contains(focus_name))emit update_focus(storage.value(focus_name));
}


void MainWindow::on_obj_list_move_itemClicked(QListWidgetItem *item)
{
    focus_name=item->text();
    ui->btn_follow->setEnabled(true);
    if(storage_move.contains(focus_name))emit update_focus(storage_move.value(focus_name));
}


void MainWindow::on_btn_follow_clicked(bool checked)
{
    if(focus_name==""){
        return;
    }
    if(checked){
        ui->btn_follow->setText("unfollow object");
        follower->scenario="follow";
    }else{
        ui->btn_follow->setText("follow object");
//        sender->sendCmd("stop");
        follower->scenario="";
    }
}

void MainWindow::on_btn_search_clicked(bool checked)
{
    if(checked){
        ui->btn_search->setText("stop search object");
        follower->scenario="search";
    }else{
        ui->btn_search->setText("search objects");
//        sender->sendCmd("stop");
        follower->scenario="";
        follower->pause();
    }
}


void MainWindow::on_btn_dist_clicked(bool checked)
{
    if(checked){
        ui->btn_dist->setText("stop calculate distance");
        sender->sendCmd("start_distance");
    }else{
        ui->btn_dist->setText("calculate distance");
        sender->sendCmd("stop_distance");
    }
}



void MainWindow::on_debug_record_clicked(bool checked)
{
    if(checked){
        ui->debug_record->setText("stop debug record");
        sender->sendCmd("record","1");
    }else{
        ui->debug_record->setText("start debug record");
        sender->sendCmd("record","0");
    }
}


void MainWindow::on_self_record_clicked(bool checked)
{
    if(checked){
        ui->self_record->setText("stop self record");
        recorder->start(this,30);
    }else{
        ui->self_record->setText("start self record");
        recorder->stop();
    }

}

