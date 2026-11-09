#pragma once

#include <QObject>
#include <QString>
#include <QDebug>
#include <QTimer>
#include <QMutex>
#include <QMap>
#include <QVector>
#include <QFile>
#include <atomic>
#include <functional>
#include <QJsonObject>
#include <QQueue>
#include <gst/gst.h>

class StreamRecorder : public QObject
{
    Q_OBJECT

public:
    explicit StreamRecorder(QObject *parent=nullptr);
    ~StreamRecorder();

    bool start(const QString &url, const QString &sessionBasePath=QString());
    void stop();
    static bool checkStreamAvailable(const QString &url,int timeoutMs=4000,
                                     QString *errorMessage=nullptr);

    bool isRecording() const { return m_running; }
    QString outputBasePath() const { return m_outputBasePath; }
    QVector<quint64> recordedTimestamps() const;
    QVector<QJsonObject> recordedTelemetry() const;
    void setTelemetryProvider(std::function<QJsonObject(quint64)> provider);
    bool frameCountVerified() const { return m_frameCountVerified; }
    QString lastError() const { return m_lastError; }

signals:
    void frameRecorded(quint64 timestamp);
    void recordingError(const QString &message);
    void segmentLimitReached();

private:
    friend class RecordingTest;
    void checkPipeline();
    void fail(const QString &message);
    void log(const QString &message);
    void handleMessage(GstMessage *message);
    void logProgress();
    static GstPadProbeReturn onInputProbe(GstPad *, GstPadProbeInfo *, gpointer);
    static GstPadProbeReturn onDecodedProbe(GstPad *, GstPadProbeInfo *, gpointer);
    static GstPadProbeReturn onOverlayProbe(GstPad *, GstPadProbeInfo *, gpointer);
    static GstPadProbeReturn onFrameProbe(GstPad *, GstPadProbeInfo *, gpointer);
    bool buildPipeline(const QString &url);
    void destroyPipeline();

    static void onPadAdded(GstElement *src,GstPad *pad,gpointer data);
    static GstPadProbeReturn onRtpProbe(GstPad *pad,GstPadProbeInfo *info,gpointer data);

private:
    QString tag;
    QString m_outputBasePath;
    bool m_running=false;
    bool m_failed=false;
    bool m_receivedEos=false;
    bool m_frameCountVerified=false;
    bool m_haveKeyframe=false;
    GstClockTime m_segmentFirstPts=GST_CLOCK_TIME_NONE;
    std::atomic_bool m_segmentLimitRequested{false};
    QString m_lastError;
    QFile m_log;
    QTimer m_monitor;
    mutable QMutex m_mutex;
    QMap<quint64,quint64> m_rtpByPts;
    QVector<quint64> m_recordedTimestamps;
    QVector<QJsonObject> m_recordedTelemetry;
    struct PendingEncodedFrame {
        quint64 timestamp=0;
        QJsonObject telemetry;
    };
    QQueue<PendingEncodedFrame> m_pendingEncodedFrames;
    std::function<QJsonObject(quint64)> m_telemetryProvider;
    std::atomic<qint64> m_lastFrameMs{0};
    struct Progress {
        std::atomic<quint64> count{0};
        std::atomic<qint64> lastMs{0};
        void reset() { count.store(0); lastMs.store(0); }
    };
    Progress m_rtpProgress;
    Progress m_inputProgress;
    Progress m_decodedProgress;
    Progress m_overlayProgress;
    Progress m_encodedProgress;
    qint64 m_nextProgressMs = 0;

    GstElement *m_pipeline=nullptr;

    GstElement *m_source=nullptr;
    GstElement *m_depay=nullptr;
    GstElement *m_parse=nullptr;
    GstElement *m_overlay=nullptr;
    GstElement *m_sink=nullptr;
};
