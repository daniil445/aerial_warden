#ifndef MOTION_CONTROLLER_H
#define MOTION_CONTROLLER_H

#include <QWidget>
#include <QDebug>
#include <QThread>
#include <QTimer>
#include <QMessageBox>
#include <QVector2D>
#include <QVector3D>
#include <QSettings>

namespace Ui {
class motion_controller;
}

class motion_controller : public QWidget
{
    Q_OBJECT
public:
    explicit motion_controller(QWidget *parent = nullptr);
    ~motion_controller();
    double x_speed;
    double y_speed;
    QPointF mouseSpeedLimits() const;
    QPointF joystickSpeedLimits() const;
    void restorePreferences(QSettings &settings);
    void savePreferences(QSettings &settings) const;
    bool screenClickEnabled() const;
    bool stabilizationEnabled() const;

signals:
    void preferencesChanged();
    void sig_homing_xy();
    void sig_upd_axis_status(int);
    void sig_move_xy_to(double,double);
    void sig_move_to(int,double);
    void moveCommand(QString cmd, double speed, bool pressed);
    void moveToCommand(double pos_x, double pos_y, double speed_x, double speed_y);
    void update_speed_x_y(double,double);
    void send_zoom(int);
    void change_cam(int);
    void update_screen_click(int);
    void update_stabilization_click(int);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
public slots:
    void keyPressEvent(QKeyEvent* event);
    void keyReleaseEvent(QKeyEvent* event);
    void update_aim(QVector2D,QVector3D,QVector3D,QVector3D);
    void update_zoom(int zoom);
private:
    void appendPosition(const QString &name, double x, double y);
    bool m_havePosition = false;
    Ui::motion_controller *ui;
    QTimer *upd_tmr;

    double cam_zoom=1;

    double aim_x=0;
    double aim_y=0;

    bool connected=false;
};

#endif // MOTION_CONTROLLER_H
