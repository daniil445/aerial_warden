#include "target_escort.h"
#include <QSettings>

target_escort::target_escort(QObject* parent)
    : QThread(parent)
{
    objct.box=QRect(30,30,100,100);
    objct.classname=123;
    objct.id=-1;
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
    objct.id=-1;
    idenificated=false;
}

void target_escort::update_focus(Detection focus)
{
    qDebug()<<"update_focus"<< objct.classname<< objct.id<<objct.angle_center;
    if(focus.id!=-1){//pause();
        objct.kalman_init = true;
        objct.box=focus.box;
        objct.classname=focus.classname;
        objct.id=focus.id;
        objct.old=focus.old;
        objct.prec=focus.prec;
        objct.box=focus.box;
    }else{
        m_sender->sendTarget(objct.get_name(),
                             -1,
                             QPointF(0,0));
    }

}

void target_escort::update_size(QSize size)
{
    image_size=size;
}

void target_escort::update_speed_x_y(double x_s, double y_s)
{
    x_speed=x_s;
    y_speed=y_s;
    qDebug()<<"move_to object"<<x_speed<<y_speed;
}

void target_escort::update_storage()
{
    if( m_storage->value(objct.get_name()).id==-1)return;
    data_ready=true;
}

void target_escort::update_meta_pos(QVector2D ang, QVector3D pos)
{
    global_x_ang=ang.x();
    global_y_ang=ang.y();
    global_ang=QPointF(global_x_ang,global_y_ang);
}

void target_escort::update_meta(quint64 frame, int zoom)
{
    zoom_state=zoom;
}

void target_escort::run()
{
    qDebug()<<"algorythm m_running";
    while (m_running){
        if(data_ready)continue;
        data_ready=false;
        if(scenario=="follow"){
            follow();
        }else if(scenario=="search"){
            if(test){play_test();continue;}
            if(!scan){
                scan=true;
                emit move_to_object(0,0,0,y_speed);
                emit move_by_object(QString("%0|%1").arg(x_speed).arg(0));
                qDebug()<<"algorythm searched";
            }
        }else if(scenario==""){
            //            m_sender->sendTarget(objct.get_name(), QString("%1 %2").arg(0).arg(-1), QPointF(0,0));
        }
        QThread::msleep(100);
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
    msleep(100);
}

double target_escort::get_speed(double percent)
{
    QList<int> speeds={-40,-30,-20,-15,-10,-8,-5,-3,-2,-1,0,1,2,3,5,8,10,15,20,30,40};
    if (qAbs(percent) < 0.01) return 0;
    percent = qBound(-0.5, percent, 0.5);
    double norm = (percent + 0.5) / 1.0;  // [0..1]
    int index = qRound(norm * (speeds.size() - 1));
    return speeds[index]/zoom_state;
}

QPointF target_escort::get_speed(QPointF percents)
{
    QPointF speeds=QPointF(get_speed(percents.x()),get_speed(percents.y()));
    return speeds;
}

void target_escort::follow()
{
    //    update_storage();
    Detection& temp = (*m_storage)[objct.get_name()];
    QPointF future = kalman_predict( temp, 0.2);     // прогноз на 200 мс вперед
    if(temp.id==-1){
        m_sender->sendTarget(objct.get_name(), -1, QPointF(0,0));
        return;
    }

    m_sender->sendTarget(objct.get_name(), objct.id, temp.angle_center);
    //    qDebug()<<"sendTarget"<<objct.get_name()<<"="<<temp.get_name()<<temp.angle_center<<future;

    QVector2D error(temp.angle_center - global_ang);
    double dist = error.length();
    double gain = 0.15;
    double maxSpeed = x_speed;
    double speed = maxSpeed * (1.0 - exp(-gain * dist));
    qDebug()<<"move_to object"<<objct.get_name()<<temp.angle_center;
    emit move_to_object(temp.angle_center.x(),temp.angle_center.y(),speed,speed);

    //    if(!moving){
    //        emit move_to_object(temp.angle_center_projective.x(),temp.angle_center_projective.y(),5,5);
    //        moving=true;
    //    }else{
    //         qDebug()<<"algorythm euals"<<temp.angle_center.toPoint()<<global_ang.toPoint();
    //        if(temp.angle_center.toPoint()==global_ang.toPoint())moving=false;
    //    }
    if(follow_zoom)follow_by_zoom(objct);
}

void target_escort::follow_by_zoom(Detection target)
{
    double diagonal = std::sqrt(target.box.width() * target.box.width() + target.box.height() * target.box.height());
    QSettings settings("config.ini", QSettings::IniFormat);
    QVariantList temp =settings.value("obj_size").toList();
    zoom_target=diagonal/zoom_state/(double)temp[target.classname].toInt();
    qDebug()<<"algorythm size"<<diagonal<<target.classname<<temp[target.classname].toInt()<<zoom_target;
    if(diagonal<temp[target.classname].toInt())emit zoom_to_object(zoom_state+1);
    //        else emit zoom_to_object(0);
    //        zoom_to_object(0);
}
