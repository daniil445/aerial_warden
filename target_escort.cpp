#include "target_escort.h"

#include <QSettings>

target_escort::target_escort(QObject* parent)
        : QThread(parent)
{
    objct.box=QRect(30,30,100,100);
    objct.classname=123;
    objct.id=0;
    first.id=-1;
}

target_escort::~target_escort()
{
    stop();
    wait();
}

void target_escort::stop()
{
    m_running = false;
}

void target_escort::pause()
{
    scan=false;
    play_scan=false;
    play_scan_y_pos=false;
    first.id=-1;
    idenificated=false;
}

void target_escort::link_storages(QMap<QString, Detection> * s_move, QMap<QString, Detection> * s_det)
{
    storage=*s_det;
    storage_move=*s_move;
}


void target_escort::update_focus(Detection focus)
{
    if(focus.id==-1)//pause();
    objct.box=focus.box;
    objct.classname=focus.classname;
    objct.id=focus.id;
    objct.old=focus.old;
    objct.prec=focus.prec;
    objct.box=focus.box;
//    if(objc1t.history.length()>5) objct.history.pop_front();
//    objct.history.append(focus.get_center());
//    qDebug()<<" objct.history"<< objct.history;
}

void target_escort::update_size(QSize size)
{
    image_size=size;
}

void target_escort::update_speed_x_y(double x_s, double y_s)
{
    x_speed=x_s;
    y_speed=y_s;
}

void target_escort::update_list(QVector<Detection> objs)
{
    foreach (Detection var, objs) {
        if(var.classname==-1){
            if(first.id!=-1){
                if(var.id == first.id)first.box=var.box;
            }
            first=var;
            break;
        }
    }
}

void target_escort::update_meta_pos(QVector2D ang, QVector3D pos)
{
//    qDebug()<<"global_"<<ang;
    global_x_ang=ang.x();
    global_y_ang=ang.y();
}

void target_escort::update_meta(int frame, double zoom)
{
    zoom_state=zoom;
}

void target_escort::run()
{
    qDebug()<<"algorythm m_running";
    while (m_running){
        if(scenario=="follow"){
            follow(objct);
        }else if(scenario=="search"){
            if(test){play_test();continue;}
            if(!scan){
                scan=true;
                emit move_to_object(0,0,0,y_speed);
                emit move_by_object(QString("%0|%1").arg(x_speed).arg(0));
                qDebug()<<"algorythm searched";
            }
            if(first.id!=-1){
                follow(first);
                emit zoom_to_object(zoom_state+1);
                qDebug()<<"algorythm detected";
                if(idenificated){
                    qDebug()<<"algorythm idenificated";
                    emit zoom_to_object(zoom_state-1);
                }
            }else{
                emit move_to_object(0,0,0,0);
//                emit zoom_to_object(0);
            }
        }
        msleep(1000);
    }
}

void target_escort::play_test()
{
    qDebug()<<"play_test"<<zoom_state<<play_scan<<play_scan_y_pos<<global_y_ang<<pow(global_y_ang-340.0,2);
    if(!play_scan){
        play_scan=true;
        emit move_to_object(0,-20,0,y_speed);
        emit zoom_to_object(zoom_state-1);
        qDebug()<<"play_ go to y";
    }else{
        if(play_scan_y_pos){
            if(play_scan_x_pos){
                qDebug()<<"zooming"<<zoom_state;
                //if(zoom_state>20)emit zoom_to_object(0);
            }else{
                if(pow(global_x_ang-50,2)<5){
                    emit move_to_object(0,0,0,0);
                    emit zoom_to_object(zoom_state+1);
                    play_scan_x_pos=true;
                }
//                 qDebug()<<"zooming"<<zoom_state;
            }
        }else{
            if(pow(global_y_ang-340.0,2)<0.01){
                emit move_by_object(QString("%0|%1").arg(x_speed).arg(0));
                 qDebug()<<"go to y";
                 play_scan_y_pos=true;
            }else{

            }
            //if(zoom_state==1)emit zoom_to_object(0);
        }
    }
    msleep(1000);
}


double target_escort::get_koef(double var)
{
    QList<double> lvls={0.01,0.1,0.2,0.3,0.4,0.45};
    double koef=1;
    if(abs(var)<=lvls.value(0))koef=0;
    else if(abs(var)<=lvls.value(1))koef=0.4;
    else if(abs(var)<=lvls.value(2))koef=0.6;
    else if(abs(var)<=lvls.value(3))koef=0.9;
    else if(abs(var)<=lvls.value(4))koef=1.2;
    else if(abs(var)<=lvls.value(5))koef=1.5;
    return koef*var/abs(var);
}

void target_escort::follow(Detection target)
{
    qDebug()<<"algorythm follow"<<target.id;
    double x= (-image_size.width() /2+target.get_center().x())/image_size.toSizeF().width();
    double y= (-image_size.height()/2+target.get_center().y())/image_size.toSizeF().height();
    offset=QPointF(x,y);
    QThread::msleep(100);

    if(y_speed*get_koef(offset.y())!=prev_y_speed || x_speed*get_koef(offset.x())!=prev_x_speed){
        qDebug()<<"algorythm played "<<QString("(%0,%1)-(%2,%3)-(%4,%5)")
                  .arg(QString::number(x_speed,'d',2)).arg(QString::number(y_speed,'d',2))
                  .arg(QString::number(get_koef(offset.x()),'d',2)).arg(QString::number(get_koef(offset.y()),'d',2))
                  .arg(QString::number(offset.x(),'d',2)).arg(QString::number(offset.y(),'d',2));
        emit move_by_object(QString("%0|%1").arg(x_speed*get_koef(offset.x())).arg(y_speed*get_koef(offset.y())));
        prev_x_speed=x_speed*get_koef(offset.x());
        prev_y_speed=y_speed*get_koef(offset.y());
    }
    double diagonal = std::sqrt(target.box.width() * target.box.width() + target.box.height() * target.box.height());
    QSettings settings("config.ini", QSettings::IniFormat);
    QVariantList temp =settings.value("obj_size").toList();
    qDebug()<<"algorythm size"<<diagonal<<target.classname;
    if(follow_zoom){
        if(diagonal<temp[target.classname].toInt())emit zoom_to_object(zoom_state+1);
//        else emit zoom_to_object(0);
    }else{
//        zoom_to_object(0);
    }
}
