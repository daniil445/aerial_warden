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
}

void target_escort::update_meta(int frame, double zoom)
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
            follow(objct);
        }else if(scenario=="search"){
            if(test){play_test();continue;}
            if(!scan){
                scan=true;
                emit move_to_object(0,0,0,y_speed);
                emit move_by_object(QString("%0|%1").arg(x_speed).arg(0));
                qDebug()<<"algorythm searched";
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

void target_escort::follow(Detection targe)
{
    update_storage();
    QPointF filtered = kalman_update( objct, objct.angle_center, 1.0/30.0);
    QPointF future = kalman_predict( objct, 0.2);     // прогноз на 200 мс вперед
    qDebug()<<"algorythm future"<<future;
       // emit move_to_object(future.x(),future.y(),5,5);
    emit move_to_object(objct.angle_center.x(),objct.angle_center.y(),5,5);
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

void target_escort::kalman_init(Detection& det)
{
    det.kalman_init = true;

    det.x[0] = det.angle_center.x();
    det.x[1] = det.angle_center.y();
    det.x[2] = 0.0;
    det.x[3] = 0.0;

    memset(det.P,0,sizeof(det.P));

    det.P[0][0]=10;
    det.P[1][1]=10;
    det.P[2][2]=100;
    det.P[3][3]=100;
}

QPointF target_escort::kalman_update( Detection& det, QPointF measurement, double dt)
{
    if(!det.kalman_init)
    {
        det.angle_center = measurement;
        kalman_init(det);
        return measurement;
    }

    //--------------------------------
    // F
    //--------------------------------

    double F[4][4]={
        {1,0,dt,0},
        {0,1,0,dt},
        {0,0,1,0},
        {0,0,0,1}
    };

    //--------------------------------
    // Q
    //--------------------------------

    double q=0.01;

    double Q[4][4]={
        {q,0,0,0},
        {0,q,0,0},
        {0,0,q,0},
        {0,0,0,q}
    };

    //--------------------------------
    // prediction
    //--------------------------------

    double xp[4];

    xp[0]=det.x[0]+dt*det.x[2];
    xp[1]=det.x[1]+dt*det.x[3];
    xp[2]=det.x[2];
    xp[3]=det.x[3];

    double FP[4][4]={0};

    for(int i=0;i<4;i++)
        for(int j=0;j<4;j++)
            for(int k=0;k<4;k++)
                FP[i][j]+=F[i][k]*det.P[k][j];

    double Pp[4][4]={0};

    for(int i=0;i<4;i++)
        for(int j=0;j<4;j++)
            for(int k=0;k<4;k++)
                Pp[i][j]+=FP[i][k]*F[j][k];

    for(int i=0;i<4;i++)
        Pp[i][i]+=Q[i][i];

    //--------------------------------
    // measurement
    //--------------------------------

    double z0=measurement.x();
    double z1=measurement.y();

    double r=0.05;

    double y0=z0-xp[0];
    double y1=z1-xp[1];

    double S00=Pp[0][0]+r;
    double S01=Pp[0][1];
    double S10=Pp[1][0];
    double S11=Pp[1][1]+r;

    double detS=S00*S11-S01*S10;

    if(fabs(detS)<1e-9)
        detS=1e-9;

    double invS00=S11/detS;
    double invS01=-S01/detS;
    double invS10=-S10/detS;
    double invS11=S00/detS;

    //--------------------------------
    // K
    //--------------------------------

    double K[4][2];

    for(int i=0;i<4;i++)
    {
        K[i][0]=Pp[i][0]*invS00 + Pp[i][1]*invS10;
        K[i][1]=Pp[i][0]*invS01 + Pp[i][1]*invS11;
    }

    //--------------------------------
    // state update
    //--------------------------------

    det.x[0]=xp[0]+K[0][0]*y0+K[0][1]*y1;
    det.x[1]=xp[1]+K[1][0]*y0+K[1][1]*y1;
    det.x[2]=xp[2]+K[2][0]*y0+K[2][1]*y1;
    det.x[3]=xp[3]+K[3][0]*y0+K[3][1]*y1;

    //--------------------------------
    // covariance update
    //--------------------------------

    double KH[4][4]={0};

    for(int i=0;i<4;i++)
    {
        KH[i][0]=K[i][0];
        KH[i][1]=K[i][1];
    }

    double I_KH[4][4]={0};

    for(int i=0;i<4;i++)
    {
        I_KH[i][i]=1;
        for(int j=0;j<4;j++)
            I_KH[i][j]-=KH[i][j];
    }

    for(int i=0;i<4;i++)
        for(int j=0;j<4;j++)
        {
            det.P[i][j]=0;
            for(int k=0;k<4;k++)
                det.P[i][j]+=I_KH[i][k]*Pp[k][j];
        }

    return QPointF(det.x[0],det.x[1]);
}

QPointF target_escort::kalman_predict( const Detection& det, double future_dt)
{
    double az =
        det.x[0] +
        det.x[2]*future_dt;

    double el =
        det.x[1] +
        det.x[3]*future_dt;

    return QPointF(az,el);
}
