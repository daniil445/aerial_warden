#pragma once

#include "UDPReceiver.h"
#include <QKeyEvent>
#include <QListWidget>
#include <QSettings>


#include <QMainWindow>
#include <RtspReceiver.h>
#include <target_escort.h>
#include <conn_settings.h>
#include <VideoRecorder.h>
#include <QTimer>


QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    QImage composeRecorderFrame();

signals:
    void send_mess(QJsonObject& obj);
    void moveCommand(QString cmd, bool pressed);
    void update_focus(Detection focus);
    void update_storage(Detection stored);
    void show_movement(bool);
    void update_zoom(double);
public slots:
    void try_to_connect(QStringList);
    void update_meta(quint64 frame, int zoom);
    void update_meta_pos(QVector2D,QVector3D);
    void update_distance(double);
    void sendMoveCommand(const QString& cmd, double speed, bool pressed);
    void sendMoveToCommand(double pos_x, double pos_y, double speed_x, double speed_y);
    void sendMoveToCommandPos(QPointF pos);
    void update_list();
    void recordTick();

protected:
private slots:
    void on_btn_follow_clicked(bool checked);
    void on_btn_search_clicked(bool checked);
    void on_btn_dist_clicked(bool checked);
    void on_obj_list_itemClicked(QListWidgetItem *item);

    void on_obj_list_move_itemClicked(QListWidgetItem *item);

    void on_debug_record_clicked(bool checked);

    void on_self_record_clicked(bool checked);

private:
    Ui::MainWindow *ui;

    conn_settings* dialog_sett;
    RtspReceiver* rtcp_receiver_RGB;
    RtspReceiver* rtcp_receiver_IR;
    CommandReceiver* UDP_receiver;
    CommandSender* sender;
    target_escort* follower;
    VideoRecorder* recorder;

    QMap <QString,Detection> storage;
    QMap <QString,Detection> storage_move;

    QTimer recordTimer;
    QImage m_recordFrame;
    QSize m_recordSize;

    double zoom=1;
    double temp_speed_x=0;
    double temp_speed_y=0;
    QString main_ip;
    QString main_stream="rgb_zoom_fix";
    QString focus_name;
    int main_port;
    int meta_port;


};
