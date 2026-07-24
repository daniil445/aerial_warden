#pragma once
#include <QWidget>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QImage>
#include <QMutex>
#include <QRect>
#include <QQueue>
#include <QVector3D>
#include <QVector2D>
#include <QPainter>
#include <QElapsedTimer>
#include "func_and_structure.h"

namespace Ui {
class VideoWidget;
}

class VideoWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit VideoWidget(QWidget* parent = nullptr);
    QImage temp_image;

    bool motion_visible =true;
    bool motion_planes =true;
    bool motion_drones =true;
    bool motion_birds =true;
    bool motion_cars =true;
    bool motion_mans =true;
    bool show_aim =true;
    bool show_degree =true;
    bool show_text =true;
    bool enable_click=true;

signals:
    void update_list();
    void update_size(QSize size);
    void set_meta_f_z(quint64 frame, int zoom);
    void set_meta_a_p(QVector2D,QVector3D);
    void set_meta_d(double);
    void imageClicked(QPoint pos);
    void moveToCommand(QPointF);

public slots:
    void setFrame(quint64 name,const QImage& img);
    void setMeta(const QJsonObject& obj);
    void update_focus(Detection);
    void link_storages(QMap<QString, Detection>* move, QMap<QString, Detection>* storage)
    {
        m_storage = storage;
        m_storage_move = move;    
    }

protected:
    void paintGL() override;
    void initializeGL() override;
    void mousePressEvent(QMouseEvent *event) override;
private slots:
    void onImageClicked(QPoint p);
private:
    QMap<QString, Detection>* m_storage_move = nullptr;
    QMap<QString, Detection>* m_storage = nullptr;

    QMutex m_mutex;

    QMutex m_queueMutex;
    int MAX_QUEUE=5;
    // QQueue<QueuedMeta> m_metaQueue;
    QQueue<QueuedFrame> m_frameQueue;
    QString debugFrameQueue()
    {
        QMutexLocker locker(&m_queueMutex);
        QStringList ids;
        for (const QueuedFrame& frame : m_frameQueue)
        {
            ids << QString::number(frame.frameId);
        }
        return ids.join(" ");
    }
    quint64 d_frame_time=0;
    quint64 m_frame_time=-1;
    QSize curr_size;
    int save_frame_count=30;
    int zoom=-1;


    QVector3D st_angle=QVector3D(0,0,0);
    QVector2D ptz_angle=QVector2D(0,0);
    QVector2D ptz_speed=QVector2D(0,0);
    QVector3D st_pos=QVector3D(0,0,0);
    QPointF click_pos;
    double st_dist=-1;

    QElapsedTimer statTimer;
    int frameCounter = 0;
    int metaCounter = 0;
    int paintCounter = 0;
    double fps = 0;
    double pps = 0;
    double dps = 0;

    QVector<Detection> last_detection;
    Detection main_obj;
    Detection target_obj;
    QColor green_overlay=QColor(0, 0xdd, 0, 0xaa);
    QColor red_overlay=QColor(0xdd, 0, 0, 0xaa);
    QColor gray_overlay=QColor(0xdd, 0xdd, 0xdd, 0xff);
    void paint_overlay(QPainter *);
    void paint_ai_objs(QPainter *,QVector<Detection>);
    void draw_aim(QPainter *painter,QPoint,QPointF coef=QPointF(1,1),QColor col=QColor(0, 0xdd, 0, 0xaa));
    // void draw_azimuth_scale(QPainter*, double);
    // void drawPitchScale(QPainter*, double);
    void draw_text(QPainter *painter);
    void work_with_storage(QJsonArray ai);
    void update_storage(Detection ogj);
    QImage findFrameById(qint64 frameId);

    inline void draw_degree( QPainter* painter, const QVector2D& camera_angle, const QPointF& p1_deg, const QPointF& p2_deg, const QPointF& fov, const QSize& size, QColor col = QColor(0xdd,0,0xdd,0xaa))
    {
        QPoint p1 = globalToLocal(camera_angle, p1_deg, fov, size);
        QPoint p2 =  globalToLocal( camera_angle, p2_deg, fov, size);
        painter->save();
        painter->setPen(QPen(col,2));
        painter->drawLine(p1.x(),p1.y(),p2.x(),p1.y());
        painter->drawLine(p2.x(),p1.y(),p2.x(),p2.y());
        painter->restore();
    }

    void draw_test_marker(QPainter* painter,  QVector2D camera_angle, QPointF marker_angle, QPointF fov, QSize image_size, QColor col=QColor(0xdd, 0, 0xdd, 0xaa)){
        QPoint p = globalToLocal( camera_angle, marker_angle, fov, image_size);
        if(p.x() < -100 || p.x() > image_size.width()+100) return;
        if(p.y() < -100 || p.y() > image_size.height()+100) return;
        painter->save();
        painter->setPen(QPen(col,2));
        int r = 25;
        int r2 = 12;
        int r3 = 5;
        painter->drawLine( p.x()-r3, p.y(), p.x()+r3, p.y());
        painter->drawLine( p.x(), p.y()-r3, p.x(),p.y()+r3);

        painter->drawLine( p.x()-r, p.y(), p.x()-r2, p.y());
        painter->drawLine( p.x()+r2, p.y(), p.x()+r, p.y());

        painter->drawLine( p.x(), p.y()-r, p.x(),p.y()-r2);
        painter->drawLine( p.x(), p.y()+r, p.x(),p.y()+r2);

        // painter->drawEllipse(p,6,6);
        painter->drawText( p.x()+10, p.y()-10, QString("%1 %2") .arg(marker_angle.x(),0,'f',2).arg(marker_angle.y(),0,'f',2));
        painter->restore();
    }
    void drawMotionVector(QPainter* painter, QVector2D speed, QColor col=QColor(0xdd, 0xdd, 0, 0xaa)){
        if (abs(speed.x())<1 && abs(speed.y())<1) return;
        if (qFuzzyIsNull(speed.x()) && qFuzzyIsNull(-speed.y())) return;
        QPoint center(width()/2, height()/2);
        const double maxLen = 100.0;
        double len = std::hypot(speed.x(), speed.y());
        double nx = speed.x();
        double ny = speed.y();
        if (len > 0.0001){
            nx /= len;
            ny /= len;
        }
        double drawLen = std::min(len * 5.0, maxLen);
        QPoint end(center.x() + nx * drawLen, center.y() - ny * drawLen);
        painter->setPen(QPen(col, 1));
        painter->drawLine(center, end);
        double angle = atan2(center.y() - end.y(),end.x() - center.x());
        const double head = 12;
        QPointF p1(end.x() - head*cos(angle - M_PI/6),end.y() + head*sin(angle - M_PI/6));
        QPointF p2(end.x() - head*cos(angle + M_PI/6),end.y() + head*sin(angle + M_PI/6));
        painter->drawLine(end, p1.toPoint());
        painter->drawLine(end, p2.toPoint());
        painter->drawText(end + QPoint(10, -10),QString("%1 %2").arg(speed.x(),0,'f',1).arg(speed.y(),0,'f',1));
    }

    void drawVector(QPainter* painter, QPoint start, QPoint stop, QPointF coef, QColor col=QColor(0x00, 0xdd, 0, 0xaa)){
        qDebug()<<"len"<<abs((start-stop).manhattanLength());
        if (abs((start-stop).manhattanLength())<3) return;
        painter->setPen(QPen(col, 1));
        start= QPointF(start.x()*coef.x(),start.y()*coef.y()).toPoint();
        stop= QPointF(stop.x()*coef.x(),stop.y()*coef.y()).toPoint();
        painter->drawLine(start, stop);
        double angle = atan2(start.y() - stop.y(),stop.x() - start.x());
        const double head = 12;
        QPointF p1(stop.x() - head*cos(angle - M_PI/6),stop.y() + head*sin(angle - M_PI/6));
        QPointF p2(stop.x() - head*cos(angle + M_PI/6),stop.y() + head*sin(angle + M_PI/6));
        painter->drawLine(stop, p1.toPoint());
        painter->drawLine(stop, p2.toPoint());
//        painter->drawText(stop + QPoint(10, -10),QString("%1 %2").arg(stop.x()-start.x(),0,'f',1).arg(stop.y()-start.y(),0,'f',1));
    }

    void draw_azimuth_scale(QPainter* painter, double headingDeg, double cameraZoom){
        const int y = 30;
        const int scaleWidth = width() * 0.8;
        const int centerX = width() / 2;
        const int leftX = centerX - scaleWidth / 2;

        painter->setPen(QPen(green_overlay, 2));
        painter->drawLine(leftX, y, leftX + scaleWidth, y);

        QFontMetrics fm(painter->font());

        // адаптивный шаг делений
        double step = cameraZoom / 10.0; // 10 делений на экран
        if (step < 1) step = 1;
        if (step > 30) step = 30;

        for (double deg = -180; deg <= 540; deg += step)
        {
            double delta = deg - headingDeg;

            // нормализация угла
            while (delta > 180) delta -= 360;
            while (delta < -180) delta += 360;

            if (std::abs(delta) > getHFOV(cameraZoom) / 2.0)
                continue;

            double norm = delta / getHFOV(cameraZoom);
            int x = centerX + norm * scaleWidth;

            bool major = (fmod(deg, step * 3) < 0.001);

            int tickHeight = major ? 15 : 8;
            painter->drawLine(x, y, x, y + tickHeight);

            if (major)
            {
                int normDeg = ((int)deg) % 360;
                if (normDeg < 0) normDeg += 360;

                QString text;

                switch (normDeg)
                {
                case 0:   text = "N";  break;
                case 45:  text = "NE"; break;
                case 90:  text = "E";  break;
                case 135: text = "SE"; break;
                case 180: text = "S";  break;
                case 225: text = "SW"; break;
                case 270: text = "W";  break;
                case 315: text = "NW"; break;
                default:
                    text = QString::number(normDeg);
                }

                int tw = fm.horizontalAdvance(text);
                painter->drawText(x - tw / 2,
                                  y + tickHeight + fm.height(),
                                  text);
            }
        }

        // центр (куда смотрит камера)
        painter->setPen(QPen(red_overlay, 3));
        painter->drawLine(centerX, y - 10, centerX, y + 15);

        // текст угла
        painter->setPen(QPen(gray_overlay, 3));
        painter->drawText(centerX - 46,
                          y - 15,
                          QString::number(headingDeg, 'f', 2) + "°");
    }

    void drawPitchScale(QPainter* painter,double pitchDeg,double cameraZoom)
    {
        const int x = width() - 50;
        const int scaleHeight = height() * 0.8;
        const int centerY = height() / 2;
        const int topY = centerY - scaleHeight / 2;

        double vfov = getVFOV(cameraZoom); // ВАЖНО

        painter->setPen(QPen(green_overlay, 2));
        painter->drawLine(x, topY, x, topY + scaleHeight);

        QFontMetrics fm(painter->font());

        // нормализация углов камеры
        while (pitchDeg > 180) pitchDeg -= 360;
        while (pitchDeg < -180) pitchDeg += 360;

        for (double deg = -90; deg <= 90; deg += 2.5)
        {
            double delta = deg - pitchDeg;

            if (std::abs(delta) > vfov / 2.0)
                continue;

            // ===== PROJECTION (главное отличие) =====
            double yNorm =
                tan(qDegreesToRadians(delta)) /
                tan(qDegreesToRadians(vfov / 2.0));

            int y = centerY - yNorm * scaleHeight / 2.0;

            bool major = (fmod(deg, 10.0) < 0.001);

            int tickLen = major ? 15 : 8;

            painter->drawLine(x - tickLen, y, x, y);

            if (major)
            {
                QString text = QString::number(deg);

                painter->drawText(
                    x - tickLen - fm.horizontalAdvance(text) - 5,
                    y + fm.height() / 3,
                    text);
            }
        }

        // центр (0 pitch камеры)
        painter->setPen(QPen(red_overlay, 3));
        painter->drawLine(x - 15, centerY, x + 10, centerY);

        // текущее значение
        painter->setPen(QPen(gray_overlay, 3));

        double absAngle = std::abs(pitchDeg);
        int deg = static_cast<int>(absAngle);
        double min_full = (absAngle - deg) * 60.0;
        int min = static_cast<int>(min_full);
        double sec = (min_full - min) * 60.0;

        if (pitchDeg < 0) deg = -deg;

        painter->drawText(x + 10, centerY - 15,
                          QString("%1°").arg(pitchDeg, 0, 'f', 1));

        painter->drawText(x + 10, centerY + 5,
                          QString("%1'").arg(min_full, 0, 'f', 1));

        painter->drawText(x + 10, centerY + 25,
                          QString("%1''").arg(sec, 0, 'f', 1));
    }
};
