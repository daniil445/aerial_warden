#pragma once

#include "UDPReceiver.h"
#include <QKeyEvent>
#include <QListWidget>
#include <QSettings>


#include <QMainWindow>
#include <QFile>
#include <QTimer>
#include <QElapsedTimer>
#include <QQueue>
#include <QPointer>
#include <RtspReceiver.h>
#include <conn_settings.h>
#include <VideoRecorder.h>
#include <StreamRecorder.h>
#include "UDPSender.h"
#include "func_and_structure.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
class InterfaceSettings;
}
QT_END_NAMESPACE

class JoystickController;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    QString joystickZoomCamera;

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    QImage composeRecorderFrame(const QImage& rgbFrame);

signals:
    void activeCameraChanged(int index);
    void send_mess(QJsonObject& obj);
    void moveCommand(QString cmd, bool pressed);
    void update_focus(Detection focus);
    void update_storage(Detection stored);
    void show_movement(bool);
    void update_zoom(double);
public slots:
    void shutdownAndSave();
    void try_to_connect(QStringList);
    void update_meta(quint64 frame, int zoom);
    void update_meta_pos(QVector2D,QVector3D,QVector3D,QVector3D);
    void update_distance(double);
    void sendMoveCommand(const QString& cmd, double speed, bool pressed);
    void sendMoveToCommand(double pos_x, double pos_y, double speed_x, double speed_y);
    void sendMoveToCommandPos(QPointF pos);
    void update_list();
    void recordIncomingFrame(quint64 frameIndex, quint64 timestamp, QImage frame);
    void recordStreamTelemetryFrame(quint64 timestamp);
    void cacheTelemetry(const QJsonObject& packet);

protected:
private slots:
    void on_btn_follow_clicked(bool checked);
    void on_btn_search_clicked(bool checked);
    void on_btn_dist_clicked(bool checked);
    void on_obj_list_itemClicked(QListWidgetItem *item);
    void on_obj_list_move_itemClicked(QListWidgetItem *item);
    void on_self_record_clicked(bool checked);
    void on_stream_record_clicked(bool checked);

protected:
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *object, QEvent *event) override;

private:
    bool m_primaryInfrared = false;
    bool m_rightBoxAlignmentPending = false;
    void alignRightBox();
    void scheduleRightBoxAlignment();
    QElapsedTimer m_rgbFrameAge, m_irFrameAge;
    int m_rgbFramesReceived = 0, m_irFramesReceived = 0;
    bool bothVideoStreamsLive() const;
    void updateRecordingAvailability();
    bool requireLiveVideo();
    QString m_interfaceSessionBase, m_streamSessionBase;
    int m_interfacePart = 0, m_streamPart = 0;
    quint64 m_interfacePartFirstTimestamp = 0;
    bool m_haveInterfacePartTimestamp = false;
    bool m_rotateStreamPending = false;
    QTimer m_streamPartTimer;
    bool startInterfacePart();
    void rotateStreamPart();
    void setPrimaryCamera(bool infrared);
    friend class RecordingTest;
    Ui::MainWindow *ui;
    Ui::InterfaceSettings *interfaceUi;

    conn_settings* dialog_sett;
    RtspReceiver* rtcp_receiver_RGB;
    RtspReceiver* rtcp_receiver_IR;
    CommandReceiver* UDP_receiver;
    CommandSender* sender;
    // target_escort* follower;
    VideoRecorder* recorder;
    StreamRecorder* stream_rgb_recorder;
    StreamRecorder* stream_ir_recorder;

    QMap <QString,Detection> storage;
    QMap <QString,Detection> storage_move;

    QSize m_recordSize;
    QMetaObject::Connection m_recordFrameConnection;
    QMetaObject::Connection m_streamTelemetryConnection;
    quint64 m_lastRecordedSourceIndex=0;
    bool m_haveRecordedSourceIndex=false;

    struct PendingTelemetryLine {
        quint64 sourceTimestamp=0;
    };
    struct TelemetryOutput {
        QFile file;
        QQueue<PendingTelemetryLine> pendingLines;
        int missingLines=0;
        bool firstEntry=true;
    };
    TelemetryOutput m_interfaceTelemetry;
    TelemetryOutput m_streamTelemetry;
    QMap<quint64,QJsonObject> m_telemetryPackets;
    QMap<quint64,QJsonObject> m_streamTelemetryPackets;
    bool m_collectingStreamTelemetry=false;
    bool m_shutdownStarted=false;
    QPointer<QThread> m_streamChecker;
    bool m_stoppingStream=false;
    bool m_closeAfterStream=false;
    QTimer m_streamFinishTimer;
    QElapsedTimer m_streamFinishElapsed;
    void finishStreamRecording();
    void checkStreamTelemetryTail();
    void writeStreamTelemetry();
    bool openTelemetryFile(TelemetryOutput& output,const QString& filename);
    void enqueueTelemetryFrame(TelemetryOutput& output,quint64 timestamp);
    void flushTelemetryLines(TelemetryOutput& output,bool force);
    void closeTelemetryFile(TelemetryOutput& output);
    bool findTelemetryPacket(quint64 timestamp, bool allowEarlier,
                             QJsonObject *packet) const;
    void checkAndStartStreamRecording();
    void startVerifiedStreamRecording(const QString& rgbUrl,const QString& irUrl);
    void stopStreamRecording();

    double zoom=1;
    double temp_speed_x=0;
    double temp_speed_y=0;
    QString main_ip;
    double m_rgbZoom = 0;
    qint64 m_focusReportId = 0;
    QElapsedTimer m_focusReportAge;
    int m_irZoom = 0;
    void syncActiveCameraZoom();
    void setZoomSelection(int value);
    JoystickController* m_joystick = nullptr;
    QString focus_name;
    int main_port;
    int meta_port;
    int rgb_port;
    int ir_port;

    int test_counter=-10;
};
