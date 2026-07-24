#pragma once

#include <QApplication>
#include <QVector2D>
// -------------------------------------------------------------------------------- Lists

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

// -------------------------------------------------------------------------------- convertors

inline QStringList obj_name= {"Plane","Bird","Drone","Human","Car"};
inline QVariantList obj_perfect_size= {78,114,26,36,56};

inline double getHFOV(int zoom)
{
    const double hfovWide = 66.0;
    const double hfovTele = 1.49;

    zoom = qBound(1, zoom, 50);

    double fw = 1.0 / tan(qDegreesToRadians(hfovWide * 0.5));
    double ft = 1.0 / tan(qDegreesToRadians(hfovTele * 0.5));

    double f = fw + zoomT[zoom - 1] * (ft - fw);

    return qRadiansToDegrees(2.0 * atan(1.0 / f));
}

inline double getVFOV(int zoom)
{
    const double vfovWide = 40.3;
    const double vfovTele = 0.84;

    zoom = qBound(1, zoom, 50);

    double fw = 1.0 / tan(qDegreesToRadians(vfovWide * 0.5));
    double ft = 1.0 / tan(qDegreesToRadians(vfovTele * 0.5));

    double f = fw + zoomT[zoom - 1] * (ft - fw);

    return qRadiansToDegrees(2.0 * atan(1.0 / f));
}

inline QPointF getFOV(int zoom)
{
    return QPointF(getHFOV(zoom),getVFOV(zoom));
}

inline QPoint globalToLocal( const QVector2D& camera_angle, const QPointF& target_angle, const QPointF& fov, const QSize& size)
{
    double dAz = target_angle.x() - camera_angle.x();
    double dEl = target_angle.y() - camera_angle.y();
    while (dAz > 180.0) dAz -= 360.0;
    while (dAz < -180.0) dAz += 360.0;
    double nx = tan(qDegreesToRadians(dAz)) / tan(qDegreesToRadians(fov.x()/2.0));
    double ny = tan(qDegreesToRadians(dEl)) / tan(qDegreesToRadians(fov.y()/2.0));
    int px = qRound(size.width ()*0.5 + nx*size.width ()*0.5);
    int py = qRound(size.height()*0.5 - ny*size.height()*0.5);
    return QPoint(px, py);
}

inline QPointF localToGlobal( const QVector2D& camera_angle, const QPoint& pixel, const QPointF& fov, const QSize& size)
{
    double nx =(pixel.x() - size.width()*0.5) /(size.width()*0.5);
    double ny =(size.height()*0.5 - pixel.y()) /(size.height()*0.5);
    double dAz = qRadiansToDegrees( atan(nx *tan(qDegreesToRadians(fov.x()/2.0))));
    double dEl = qRadiansToDegrees( atan( ny * tan(qDegreesToRadians(fov.y()/2.0))));
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
