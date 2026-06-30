#pragma once

#include <QApplication>
#include <QVector2D>
// -------------------------------------------------------------------------------- Lists
// enum {
//     AXIS_X = 0,
//     AXIS_Y,
//     AXIS_Z,
//     Arbitrary
// };

inline QVector<double> zoomT  ={0.0,0.22,0.335,0.41,0.47,
                                0.52,0.56,0.6,0.635,0.665,
                                0.695,0.722,0.747,0.768,0.788,
                                0.805,0.818,0.831,0.842,0.852,
                                0.862,0.8715,0.881,0.890,0.8985,
                                0.907,0.915,0.9235,0.931,0.938,
                                0.944,0.95,0.9553,0.9605,0.9645,
                                0.968,0.9715,0.975,0.9777,0.9802,
                                0.983,0.9855,0.988,0.990,0.992,
                                0.9938,0.9951,0.997,0.9991,1.0};

// -------------------------------------------------------------------------------- convertors


inline double getHFOV(double zoom)
{
    const double hfovWide = 66.0;
    const double hfovTele = 1.49;
    double fw = 1.0 /tan(qDegreesToRadians(hfovWide/2.0));
    double ft = 1.0 /tan(qDegreesToRadians(hfovTele/2.0));
    double f =fw + zoom*(ft-fw);
    return qRadiansToDegrees(2.0*atan(1.0/f));
}
inline double getVFOV(double zoom)
{
    const double vfovWide = 40.3;
    const double vfovTele = 0.84;
    double fw =1.0/tan(qDegreesToRadians(vfovWide/2.0));
    double ft =1.0/tan(qDegreesToRadians(vfovTele/2.0));
    double f =fw + zoom*(ft-fw);
    return qRadiansToDegrees(2.0*atan(1.0/f));
}
inline QPointF getFOV(double zoom)
{
    return QPointF(getHFOV(zoom),getVFOV(zoom));
}

inline QPoint globalToLocal( QVector2D camera_angle, QPointF target_angle, QPointF fov, QSize size)
{
    double dAz = target_angle.x() -camera_angle.x();
    double dEl =target_angle.y() - camera_angle.y();
    while(dAz > 180) dAz -= 360;
    while(dAz < -180) dAz += 360;
    double nx = tan(qDegreesToRadians(dAz)) / tan(qDegreesToRadians(fov.x()/2.0));
    double ny =tan(qDegreesToRadians(dEl)) / tan(qDegreesToRadians(fov.y()/2.0));
    int px = size.width()/2 + nx * size.width()/2;
    int py =size.height()/2 - ny * size.height()/2;
    return QPoint(px,py);
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
    QPoint get_center(){
        return box.center();
    }
    QPoint get_local_center(QPointF coef){
        return QPointF(box.center().x()*coef.x(),box.center().y()*coef.y()).toPoint();
    }

    QPointF angle_center;// координаты центра в углах
    QPointF angle_center_projective;
    QPoint get_global_to_local_center(QPointF global_coors, QVector2D camera_angle,QPointF fov, QSize image_size){
        double degPerPixelX =fov.x()/(double)image_size.width();
        double degPerPixelY = fov.y()/(double)image_size.height();
        double dx =(global_coors.x() - camera_angle.x())/ degPerPixelX;
        double dy =(global_coors.y() - camera_angle.y()) / degPerPixelY;
        int px =image_size.width()/2 + dx;
        int py =image_size.height()/2 + dy;
        //        qDebug()<<"get_global_to_local_center"<<get_name()<<image_size/2<<dx<<dy<<px<<py;
        return QPoint(px,py);
    }
    void set_angle_center_projective(QVector2D camera_angle, QPoint pix_obj,QSize size,QPointF fov){
        double nx =(pix_obj.x() - size.width()/2.0)/ (size.width()/2.0);
        double ny =(pix_obj.y() - size.height()/2.0)/ (size.height()/2.0);
        double dAz =qRadiansToDegrees(atan(nx * tan(qDegreesToRadians(fov.x()/2))));
        double dEl = qRadiansToDegrees(atan(ny * tan(qDegreesToRadians(fov.y()/2))));
        angle_center_projective.setX(camera_angle.x() + dAz);
        angle_center_projective.setY(camera_angle.y() + dEl);
    }
    void set_angle_center(QVector2D global_coors, QPoint pix_obj, QPointF fov, QSize size){
        //        double degPerPixelX = fov.x() / (double)size.width();
        //        double degPerPixelY = fov.y() / (double)size.height();

        //        double dx = pix_obj.x() - size.width()  / 2.0;
        //        double dy = pix_obj.y() - size.height() / 2.0;

        //        double objAz = global_coors.x() + dx * degPerPixelX;
        //        double objEl = global_coors.y() + dy * degPerPixelY;

        //        angle_center = QPointF(objAz, objEl);// заполнение координат центра в углах
        //        qDebug()<<"set_angle_center"<<get_name()<<angle_center<<dx<<dy<<pix_obj;
        double fx = size.width() /(2.0*tan(qDegreesToRadians(fov.x()/2.0)));
        double fy =size.height() /(2.0*tan(qDegreesToRadians(fov.y()/2.0)));
        double dx = pix_obj.x() - size.width()/2.0;
        double dy = pix_obj.y() - size.height()/2.0;
        double dAz = qRadiansToDegrees(  atan(dx/fx));
        double dEl = qRadiansToDegrees( atan(dy/fy));

        angle_center.setX(global_coors.x()+dAz);
        angle_center.setY(global_coors.y()+dEl);
    }

    bool kalman_init = false;

    double x[4];      // az, el, vaz, vel
    double P[4][4];   // ковариация
};

struct QueuedFrame
{
    quint64 frameId = -1;
    QImage image;
};
// -------------------------------------------------------------------------------- methods


inline void kalman_init(Detection& det){
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

inline QPointF kalman_update(Detection& det, QPointF measurement, double dt){
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

inline QPointF kalman_predict(const Detection &det, double future_dt){
    double az =
        det.x[0] +
        det.x[2]*future_dt;

    double el =
        det.x[1] +
        det.x[3]*future_dt;

    return QPointF(az,el);
}
