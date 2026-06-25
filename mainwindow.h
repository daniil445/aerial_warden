#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "UDPSender.h"
#include "UDPReceiver.h"
#include "target_escort.h"

#include <QMainWindow>
#include <RtspReceiver.h>

#include <VideoWidget.h>
#include <conn_settings.h>

#include <QKeyEvent>
#include <QListWidget>
#include <QSettings>

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

signals:
    void send_mess(QJsonObject& obj);
    void moveCommand(QString cmd, bool pressed);
    void update_focus(Detection focus);
    void show_movement(bool);
    void update_zoom(double);
public slots:
    void try_to_connect(QStringList,QStringList,QStringList);
    void update_meta(int frame, double zoom);
    void update_meta_pos(QVector2D,QVector3D);
    void update_distance(double);
    void sendMoveCommand(const QString& cmd, double speed, bool pressed);
    void sendMoveToCommand(double pos_x, double pos_y, double speed_x, double speed_y);
    void update_list(QVector<Detection>);
    void update_list_RGB(QVector<Detection>);
    void update_list_IR(QVector<Detection>);

protected:
//    bool eventFilter(QObject *obj, QEvent *event) override;
private slots:
    void on_btn_follow_clicked(bool checked);
    void on_btn_search_clicked(bool checked);
    void on_btn_dist_clicked(bool checked);
    void on_obj_list_itemClicked(QListWidgetItem *item);

private:
    Ui::MainWindow *ui;

    conn_settings* dialog_sett;
    VideoWidget* videoWidget; 
    RtspReceiver* rtcp_receiver_RGB;
    RtspReceiver* rtcp_receiver_IR;
    CommandReceiver* UDP_receiver;
    CommandSender* sender;
    target_escort* follower;

    QMap <QString,Detection> storage;
    QMap <QString,Detection> storage_move;

    double zoom=1;
    double temp_speed_x=0;
    double temp_speed_y=0;
    QString main_ip;
    QString main_stream="rgb_zoom_fix";
    QString focus_name;
    int main_port;
    int meta_port;

    int save_frame_count=30;
    void work_with_list(QListWidget*,QMap <QString,Detection>*,QVector<Detection>);
};
#endif // MAINWINDOW_H
