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

    bool motion_visible =false;
    bool motion_planes =true;
    bool motion_drones =true;
    bool motion_birds =true;
    bool motion_cars =true;
    bool motion_mans =true;
    bool show_aim =true;
    bool show_degree =true;
    bool show_text =true;
signals:
    void update_list();
    void update_size(QSize size);
    void set_meta_f_z(int frame, double zoom);
    void set_meta_a_p(QVector2D,QVector3D);
    void set_meta_d(double);
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
private:
    QMap<QString, Detection>* m_storage_move = nullptr;
    QMap<QString, Detection>* m_storage = nullptr;

    QMutex m_mutex;

    QMutex m_queueMutex;
    int MAX_QUEUE=3;
    // QQueue<QueuedMeta> m_metaQueue;
    QQueue<QueuedFrame> m_frameQueue;
    QImage m_image;
    quint64 d_frame_time=0;
    quint64 m_frame_time=0;
    QSize curr_size;
    int save_frame_count=30;
    double raw_zoom=0;
    double human_zoom=0;

    QVector3D st_angle=QVector3D(0,0,0);
    QVector2D ptz_angle=QVector2D(0,0);
    QVector3D st_pos=QVector3D(0,0,0);
    double st_dist=-1;

    QVector<Detection> last_detection;
    Detection main_obj;

    QColor green_overlay=QColor(0, 0xdd, 0, 0xaa);
    QColor red_overlay=QColor(0xdd, 0, 0, 0xaa);
    QColor gray_overlay=QColor(0xdd, 0xdd, 0xdd, 0xff);
    void paint_overlay(QPainter *);
    void paint_ai_objs(QPainter *,QVector<Detection>);
    void draw_aim(QPainter *painter,QPoint,QColor col=QColor(0, 0xdd, 0, 0xaa));
    void draw_azimuth_scale(QPainter*, double);
    void drawPitchScale(QPainter*, double);
    void draw_text(QPainter *painter);
    void work_with_storage(QJsonArray ai);
    void update_storage(Detection ogj);
};
