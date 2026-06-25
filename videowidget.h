#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include <QWidget>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QImage>
#include <QMutex>
#include <QRect>
#include <QQueue>
#include <QVector3D>
#include <QVector2D>

struct Global_coor
{
    QPointF graphic_pos;
    QVector3D station_pos;
    QVector3D station_angle;
    QVector2D rotator_angle;
    double distance;
};


struct Detection
{
    int classname;
    int id = -1;
    int old = 0;
    double prec = 0.0;
    QRect box;   // координаты в исходном кадре
    QList<QPointF> history;
    QPoint get_center(){
       return box.center();
    }
    QPointF angle_center;
    void set_angle_center(QPointF target_angle, QPoint pix_obj, QPoint fov, QSize size, QSize screen_size){
        double degPerPixelX = fov.x() / (double)size.width();
        double degPerPixelY = fov.y() / (double)size.height();

        double sx = screen_size.width()  / double(size.width());
        double sy = screen_size.height() / double(size.height());

        double dx = pix_obj.x()*sx - size.width()  / 2.0;
        double dy = pix_obj.y()*sy - size.height() / 2.0;
        double objAz = target_angle.x() + dx * degPerPixelX;
        double objEl = target_angle.y() + dy * degPerPixelY;
        angle_center = QPointF(objAz, objEl);
        qDebug()<<"set_angle_center"<<target_angle<<pix_obj<<dx<<dy<<angle_center;
    }
};

struct QueuedFrame
{
    quint64 frameId = -1;
    QImage image;
};

struct QueuedMeta
{
    quint64 metaId = -1;
    QVector<Detection> ai_objs;
};


namespace Ui {
class VideoWidget;
}

class VideoWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit VideoWidget(QWidget* parent = nullptr);
    QImage temp_image;
signals:
    void send_obj_list(QVector<Detection>);
    void update_size(QSize size);
    void set_meta_f_z(int frame, double zoom);
    void set_meta_a_p(QVector2D,QVector3D);
    void set_meta_d(double);
public slots:
    void setFrame(quint64 name,const QImage& img);
    void setMeta(const QJsonObject& obj);
    void update_focus(Detection);
    void show_movement(bool);
    void show_planes(bool);
    void show_drones(bool);
    void show_birds(bool);
    void show_cars(bool);
    void show_mans(bool);

protected:
    void paintGL() override;
    void initializeGL() override;
private:
    QMutex m_mutex;

    QQueue<QueuedFrame> m_frameQueue;
    QImage m_image;
    quint64 d_frame_time=0;
    QSize curr_size;
    double c_zoom=0;

//    QString st_angle;
    QVector3D st_angle=QVector3D(0,0,0);
//    QString ptz_angle;
    QVector2D ptz_angle=QVector2D(0,0);
    QVector3D st_pos=QVector3D(0,0,0);
    double st_dist=-1;

    QQueue<QueuedMeta> m_metaQueue;

    QVector<Detection> findMetaByFrameId(int );
    void paint_overlay(QPainter *);
    void paint_ai_objs(QPainter *);
    void draw_azimuth_scale(QPainter*, double);
    void drawPitchScale(QPainter*, double );
    QMutex m_queueMutex;

    QVector<Detection> last_detection;
    int MAX_QUEUE=3;
    Detection main_obj;
    bool motion_visible =false;

    bool motion_planes =true;
    bool motion_drones =true;
    bool motion_birds =true;
    bool motion_cars =true;
    bool motion_mans =true;
};

#endif // VIDEOWIDGET_H
