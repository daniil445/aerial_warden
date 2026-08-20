#pragma once

#include <QApplication>
#include <QElapsedTimer>
#include <QVector2D>
#include <QDebug>
// -------------------------------------------------------------------------------- Lists
inline void getTimeNs(int id)
{
    static QElapsedTimer timer;
    static bool initialized = false;
    static int counter1 = 0;
    static int counter2 = 0;
    static int counter3 = 0;
    if(id==0){
        counter1++;
    }else if(id==1){
        counter2++;
    }else if(id==2){
        counter3++;
    }
    if(!initialized){
        timer.start();
        initialized = true;
    }
    qDebug() << "Frame get processing " <<counter1<<counter2<<counter3<<  QString::number( timer.nsecsElapsed()/1000000000.0,'d',3)<<" s";
}

inline const QVector<double> zoomT  ={0.0,0.22,0.335,0.41,0.47,
                               0.52,0.56,0.6,0.635,0.665,
                               0.695,0.722,0.747,0.768,0.788,
                               0.805,0.818,0.831,0.842,0.852,
                               0.862,0.8715,0.881,0.890,0.8983,
                               0.907,0.915,0.9235,0.931,0.938,
                               0.944,0.95,0.9550,0.9603,0.9643,
                               0.968,0.9717,0.975,0.9778,0.9805,
                               0.983,0.9853,0.988,0.990,0.992,
                               0.9937,0.9952,0.997,0.999,1.0};

//inline const QVector<double> HFOV  ={  64.0,37.0,27.0,20.74,16.92, //4
//                                       14.29,12.52,10.98,9.94,8.94,
//                                       8.16,7.62,7.04,6.71,6.2,    //14
//                                       5.92,5.46,5.2,4.90,4.72,
//                                       4.5,4.28,4.08,3.92,3.79,    //24
//                                       3.65,3.51,3.39,3.29,3.21,
//                                       3.15,3.11,2.96,2.88,2.81,   //34
//                                       2.72,2.71,2.65,2.51,2.47,
//                                       2.37,2.33,2.31,2.20,2.2,    //44
//                                       2.14,2.08,2.02,1.98,2.16};

inline const QVector<double> HFOV  ={  64.50,37.13,26.40,20.74,17.05, //5
                                       14.40,12.59,11.02, 9.94, 9.02,
                                        8.30, 7.69, 7.11, 6.68, 6.23,    //15
                                       5.87,5.54,5.22,4.90,4.72,
                                       4.5,4.28,4.08,3.92,3.79,    //25
                                       3.65,3.51,3.39,3.29,3.21,
                                       3.15,3.11,2.96,2.88,2.81,   //35
                                       2.72,2.71,2.65,2.51,2.47,
                                       2.37,2.33,2.31,2.20,2.2,    //45
                                       2.14,2.08,2.02,1.98,2.16};

inline const QVector<double> VFOV  ={  38.36,21.94,15.54,12.04,9.88,
                                       8.34,7.48,6.44,5.7,5.24,
                                       4.82,4.32,4.14,3.88,3.54,
                                       3.44,3.22,3.12,3.02,2.82,
                                       2.52,2.52,2.48,2.46,2.34,
                                       2.24,2.20,2.12,2.08,1.96,
                                       1.88,1.86,1.80,1.78,1.78,
                                       1.76,1.72,1.64,1.6,1.58,
                                       1.54,1.46,1.4,1.36,1.34,
                                       1.34,1.32,1.28,1.24,1.22};

// -------------------------------------------------------------------------------- convertors

inline QStringList obj_name= {"Plane","Bird","Drone","Human","Car"};
inline QVariantList obj_perfect_size= {78,114,26,36,56};

inline QPoint globalToLocal( const QVector2D& camera_angle, const QPointF& target_angle, const int& zoom, const QSize& size)
{
    double dAz = target_angle.x() - camera_angle.x();
    double dEl = target_angle.y() - camera_angle.y();
    while (dAz > 180.0) dAz -= 360.0;
    while (dAz < -180.0) dAz += 360.0;
    double nx = tan(qDegreesToRadians(dAz)) / tan(qDegreesToRadians(HFOV[zoom-1]/2.0));
    double ny = tan(qDegreesToRadians(dEl)) / tan(qDegreesToRadians(VFOV[zoom-1]/2.0));
//    double nx = dAz / HFOV[zoom-1] / 2.0;
//    double ny = dEl / VFOV[zoom-1] / 2.0;
    constexpr double kx = -0.04;
    nx = nx - kx * nx * (1.0 - nx * nx);
    int px = qRound(size.width ()*0.5 + nx*size.width ()*0.5);
    int py = qRound(size.height()*0.5 - ny*size.height()*0.5);
    return QPoint(px, py);
}

inline QPointF localToGlobal( const QVector2D& camera_angle, const QPoint& pixel, const int& zoom, const QSize& size)
{
    double nx =(pixel.x() - size.width()*0.5) /(size.width()*0.5);
    double ny =(size.height()*0.5 - pixel.y()) /(size.height()*0.5);
    double dAz = qRadiansToDegrees( atan(nx * tan(qDegreesToRadians(HFOV[zoom-1]/2.0))));
    double dEl = qRadiansToDegrees( atan(ny * tan(qDegreesToRadians(VFOV[zoom-1]/2.0))));
//    double dAz =nx * HFOV[zoom-1]/2.0;
//    double dEl =ny * VFOV[zoom-1]/2.0;
    double az = camera_angle.x() + dAz;
    double el = camera_angle.y() + dEl;
    while (az >= 360.0) az -= 360.0;
    while (az <    0.0) az += 360.0;
    return QPointF(az, el);
}
// -------------------------------------------------------------------------------- struct

struct Detection
{
    int classname;
    int id = -1;
    int old = 0;
    double prec = 0.0;
    QRect box;   // координаты в исходном кадре
    QString get_name(){
        QStringList temp= {"Plane","Bird","Drone","Human","Car"};
        QString name="move "+QString::number(id);
        if(classname!=-1) name=temp[classname]+" "+QString::number(id);
        return name;
    }
    QPoint get_center()const {
        return box.center();
    }
    QPointF display_coef;
    QPoint get_local_center(){
        return QPointF(box.center().x()*display_coef.x(),box.center().y()*display_coef.y()).toPoint();
    }
    QPointF pixel_center;
    QPointF angle_center;
};

struct QueuedFrame
{
    quint64 frameId = -1;
    QImage image;
};
// -------------------------------------------------------------------------------- methods
