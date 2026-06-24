#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QJsonArray>
#include <QSettings>
#include <RtspReceiver.h>
#include <VideoWidget.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->cb_show_moves, &QCheckBox::toggled, ui->openGLWidget_RGB, &VideoWidget::show_movement);
    connect(ui->cb_show_planes, &QCheckBox::toggled,ui->openGLWidget_RGB, &VideoWidget::show_planes);
    connect(ui->cb_show_drones, &QCheckBox::toggled,ui->openGLWidget_RGB, &VideoWidget::show_drones);
    connect(ui->cb_show_birds, &QCheckBox::toggled, ui->openGLWidget_RGB, &VideoWidget::show_birds);
    connect(ui->cb_show_cars, &QCheckBox::toggled,  ui->openGLWidget_RGB, &VideoWidget::show_cars);
    connect(ui->cb_show_mans, &QCheckBox::toggled,  ui->openGLWidget_RGB, &VideoWidget::show_mans);



    dialog_sett= new conn_settings;
    connect(dialog_sett, &conn_settings::connect_to, this, &MainWindow::try_to_connect);
    connect(ui->actionSettings, &QAction::triggered, dialog_sett, &QWidget::show);
//    connect(ui->actionInitialize_rotator, &QAction::triggered, sender, &CommandSender::sendCmd);
    connect(ui->actionInitialize_rotator, &QAction::triggered,[=]() {sender->sendCmd("init");});


    rtcp_receiver_RGB = new RtspReceiver(this);
    connect(rtcp_receiver_RGB, &RtspReceiver::frameReady, ui->openGLWidget_RGB, &VideoWidget::setFrame);
    connect(ui->openGLWidget_RGB,&VideoWidget::send_obj_list,this, &MainWindow::update_list_RGB);

    rtcp_receiver_IR = new RtspReceiver(this);
    connect(rtcp_receiver_IR, &RtspReceiver::frameReady, ui->openGLWidget_IR, &VideoWidget::setFrame);
    connect(ui->openGLWidget_IR,&VideoWidget::send_obj_list,this, &MainWindow::update_list_IR);


    sender = new CommandSender(this);
    connect(ui->zoom_slider, &QSlider::sliderReleased, [=]() {sender->sendZoom(main_stream,ui->zoom_slider->value());});


    follower= new target_escort(this);
    follower->link_storages(&storage_move,&storage);
    connect(ui->alg_zoom, &QCheckBox::toggled, [=](bool var) {follower->follow_zoom=var;});
    connect(ui->openGLWidget_RGB, &VideoWidget::update_size,follower, &target_escort::update_size);
    connect(follower, &target_escort::move_by_object, sender, &CommandSender::sendMove);
    connect(follower,&target_escort::move_to_object,this,&MainWindow::sendMoveToCommand);
    connect(this, &MainWindow::update_focus,follower, &target_escort::update_focus);
    connect(ui->openGLWidget_RGB,&VideoWidget::send_obj_list,follower, &target_escort::update_list);
    connect(follower, &target_escort::zoom_to_object, [=](double val) {sender->sendZoom(main_stream,val);});

    follower->start();

    connect(ui->controls, &motion_controller::moveCommand,this,&MainWindow::sendMoveCommand);
    connect(ui->controls, &motion_controller::moveToCommand,this,&MainWindow::sendMoveToCommand);

    connect(ui->controls, &motion_controller::update_speed_x_y,follower,&target_escort::update_speed_x_y);

    UDP_receiver = new CommandReceiver(this);
    connect(this,&MainWindow::update_zoom,ui->controls, &motion_controller::update_zoom);
    connect(UDP_receiver, &CommandReceiver::metaObjectsReceived,ui->openGLWidget_RGB, &VideoWidget::setMeta);
    connect(ui->openGLWidget_RGB, &VideoWidget::set_meta_f_z,this,&MainWindow::update_meta);
    connect(ui->openGLWidget_RGB, &VideoWidget::set_meta_a_p,this,&MainWindow::update_meta_pos);
    connect(ui->openGLWidget_RGB, &VideoWidget::set_meta_f_z,follower,&target_escort::update_meta);
    connect(ui->openGLWidget_RGB, &VideoWidget::set_meta_a_p,follower,&target_escort::update_meta_pos);
    connect(ui->openGLWidget_RGB, &VideoWidget::set_meta_a_p,ui->controls, &motion_controller::update_aim);
    connect(ui->openGLWidget_RGB, &VideoWidget::set_meta_d,this,&MainWindow::update_distance);


    QSettings settings("config.ini", QSettings::IniFormat);
    QStringList name= {"Plane","Bird","Drone","Human","Car"};
    QVariantList perfect_size= {78,114,26,36,56};
    settings.setValue("obj_name" ,name);
    settings.setValue("obj_size" ,perfect_size);

//    qputenv("GST_DEBUG", "4");
//    qputenv("GST_PLUGIN_PATH",
//            (QCoreApplication::applicationDirPath() + "/gstreamer-1.0").toUtf8());
//    qputenv("GST_PLUGIN_SYSTEM_PATH",
//            (QCoreApplication::applicationDirPath() + "/gstreamer-1.0").toUtf8());
    qApp->installEventFilter(this);
    ui->controls->setFocus();
}

MainWindow::~MainWindow()
{
    follower->stop();
    follower->wait();
    delete ui;
}
//bool MainWindow::eventFilter(QObject *obj, QEvent *event)
//{
//    if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease )
//    {
//        qDebug()<<"eventFilter"<<event->type();
////        QCoreApplication::sendEvent(ui->controls, event);
//        return true;
//    }
//    return QObject::eventFilter(obj, event);
//}


void MainWindow::try_to_connect(QStringList url_cam_rgb,QStringList url_cam_ir,QStringList url_main)
{
    qDebug()<<"update stream"<<"rtsp://"<<url_cam_rgb[0]+":"+url_cam_rgb[1];
    qDebug()<<"update stream"<<"rtsp://"<<url_cam_ir[0]+":"+url_cam_ir[1];
    main_ip=url_main[0];
    main_port=url_main[1].toInt();
    meta_port=url_main[2].toInt();
    rtcp_receiver_RGB->setUrl("rtsp://admin:admin@"+url_cam_rgb[0]+":"+url_cam_rgb[1]+"/cam/realmonitor?channel=1&subtype=0");
    rtcp_receiver_RGB->start();
    rtcp_receiver_IR->setUrl("rtsp://admin:admin@"+url_cam_ir[0]+":"+url_cam_ir[1]+"/ir");
    rtcp_receiver_IR->start();

    qDebug()<<"update meta"<<url_main[0]<<url_main[1]<<url_main[2];

    UDP_receiver->bind(main_ip,meta_port);
    sender->sendAddress(main_ip,main_port);
    sender->sendIp();
}

void MainWindow::update_meta(int frame, double zoom)
{
    ui->l_fps->setText(QString::number(frame/100000.0,'d',3));
    ui->l_zoom->setText(QString::number(zoom,'d',3));
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

void MainWindow::sendMoveToCommand(double pos_x, double pos_y, double speed_x, double speed_y)
{
    if(speed_x==0 && speed_y==0 ) sender->sendCmd("stop");
    else sender->sendMoveTo(QString("%0|%1|%2|%3").arg(pos_x).arg(pos_y).arg(speed_x).arg(speed_y));
}

void MainWindow::update_list(QVector<Detection> obj)
{
    QVector<Detection> moves;
    QVector<Detection> objects;
    foreach (Detection var, obj) {
        if(var.classname==-1){
            moves.append(var);
        }else{
            objects.append(var);
        }
    }

    foreach (QString key, storage_move.keys()) {storage_move[key].old++;}
    foreach (QString key, storage.keys()) {storage[key].old++;}
//    qDebug()<<"storages"<<storage_move.keys()<< storage.keys();
    work_with_list(ui->obj_list_move,&storage_move,moves);
    work_with_list(ui->obj_list,&storage,objects);
}

void MainWindow::work_with_list(QListWidget * visual, QMap <QString,Detection>* storage_objects, QVector<Detection> update_objects)
{
    QString selected;
    QListWidgetItem* current = visual->currentItem();
    if (current) selected = current->text();

    foreach (Detection var, update_objects) {
        QSettings settings("config.ini", QSettings::IniFormat);
        QStringList temp =settings.value("obj_name").toStringList();
        QString name="move "+QString::number(var.id);
        if(var.classname!=-1) name=temp[var.classname]+" "+QString::number(var.id);
        bool empty =visual->findItems(name, Qt::MatchContains).isEmpty();
        if(empty){
            visual->addItem(name);
            storage_objects->insert(name,var);
//            qDebug()<<"storage_objects"<<name<<storage_objects->keys();
        }else{
            (*storage_objects)[name].old=0;
            (*storage_objects)[name].box=var.box;
            (*storage_objects)[name].history.append(var.history);
        }
    }
    for (int i = visual->count() - 1; i >= 0; --i)
    {
        QListWidgetItem* item = visual->item(i);
        if(storage_objects->contains(item->text())){
            if(storage_objects->value(item->text()).old>save_frame_count){
                storage_objects->remove(item->text());
                delete visual->takeItem(i);
            }
        }
    }
    QList<QListWidgetItem*> found = visual->findItems(selected, Qt::MatchExactly);
    if (!found.isEmpty()){
        visual->setCurrentItem(found.first());
        emit update_focus(storage_objects->value(found.first()->text()));
    }else{
        emit update_focus(Detection{});
    }
}

void MainWindow::update_list_RGB(QVector<Detection> list)
{
    update_list(list);
}

void MainWindow::update_list_IR(QVector<Detection> list)
{

}

void MainWindow::on_obj_list_currentTextChanged(const QString &currentText)
{
    if(storage.contains(currentText))emit update_focus(storage.value(currentText));
}


void MainWindow::on_btn_follow_clicked(bool checked)
{
    if(checked){
        ui->btn_follow->setText("unfollow object");
        follower->scenario="follow";
    }else{
        ui->btn_follow->setText("follow object");
        sender->sendCmd("stop");
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
        sender->sendCmd("stop");
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



