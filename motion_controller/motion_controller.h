#ifndef MOTION_CONTROLLER_H
#define MOTION_CONTROLLER_H

#include <QWidget>
#include <QDebug>
#include <QThread>
#include <QTimer>
#include <QMessageBox>
#include <QVector2D>
#include <QVector3D>

namespace Ui {
class motion_controller;
}

class motion_controller : public QWidget
{
    Q_OBJECT
public:
    explicit motion_controller(QWidget *parent = nullptr);
    ~motion_controller();

signals:
    void sig_homing_xy();
    void sig_upd_axis_status(int);
    void sig_move_xy_to(double,double);
    void sig_move_to(int,double);
    void moveCommand(QString cmd, double speed, bool pressed);
    void moveToCommand(double pos_x, double pos_y, double speed_x, double speed_y);
    void update_speed_x_y(double,double);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
public slots:
    void update_aim(QVector2D,QVector3D);
    void update_zoom(double);
private slots:


    void on_pb_move_x_clicked();

    void on_pb_move_y_clicked();

    void on_pb_all_axis_move_clicked();

private:
    Ui::motion_controller *ui;
    QTimer *upd_tmr;
    double x_speed;
    double y_speed;
    double cam_zoom=1;

    double aim_x=0;
    double aim_y=0;

    bool target_chosed=false;
    double target_x;
    double target_y;
};

#endif // MOTION_CONTROLLER_H
