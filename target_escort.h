#pragma once

#include "func_and_structure.h"
#include <QThread>
#include <QObject>

class target_escort: public QThread
{
    Q_OBJECT
public:
    explicit target_escort(QObject* parent = nullptr);
    ~target_escort();

    bool m_running = true;
    bool follow_zoom= false;
    QString scenario = "";
    void stop();
    void pause();
    void link_storages(QMap<QString, Detection>* move, QMap<QString, Detection>* storage)
    {
        m_storage = storage;
        m_storage_move = move;
    }

signals:
    void move_by_object(QString speeds);
    void move_to_object(double,double,double,double);
    void zoom_to_object(double);

public slots:
    void update_focus(Detection focus);
    void update_size(QSize size);
    void update_speed_x_y(double,double);
    void update_storage();
    void update_meta_pos(QVector2D,QVector3D);
    void update_meta(int frame, int zoom);

protected:
    void run() override;
private:
    Detection objct;
    QSize image_size;
    QPointF offset;
    double x_speed=0;
    double y_speed=0;
    double global_x_ang=0;
    double global_y_ang=0;
    QPointF global_ang;
    double zoom_state=0;
    double prev_x_speed;
    double prev_y_speed;
    double get_speed(double);
    QPointF get_speed(QPointF);

    QPointF speed;
    int zoom_target;

    void follow();
    void follow_by_zoom(Detection);
    void play_test();

    bool test=true;
    bool play_scan=false;
    bool play_scan_x_pos=false;
    bool play_scan_y_pos=false;
    bool scan=false;
    bool idenificated=false;
    bool data_ready=false;
    bool moving=false;
    quint64 m_seq = 0;

    QMap<QString, Detection>* m_storage_move = nullptr;
    QMap<QString, Detection>* m_storage = nullptr;   
};
