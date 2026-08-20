#pragma once

#include <QThread>
#include <QImage>
#include <QMutex>
#include <QString>
#include <atomic>

#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include "func_and_structure.h"

class RtspReceiver : public QThread
{
    Q_OBJECT

public:
    explicit RtspReceiver(QObject *parent = nullptr);
    ~RtspReceiver();

    void setUrl(const QString& url);
    void stop();

signals:
    void connected();
    void disconnected();
    void frameReady(quint64 time, QImage image);

protected:
    void run() override;

private:
    void deliverLatestFrame();
    static GstFlowReturn on_new_sample(GstAppSink *sink, gpointer user_data);
    static void on_new_manager(GstElement *rtspsrc, GstElement *manager, gpointer user_data);
    static void on_pad_added(GstElement *src, GstPad *pad, gpointer user_data);
    static GstPadProbeReturn on_rtp_probe(GstPad *pad, GstPadProbeInfo *info, gpointer user_data);
private:
    QString m_url="";
    std::atomic_bool m_running{false};
    bool rgb=false;

    GstElement *m_manager = nullptr;
    GstElement *m_pipeline = nullptr;
    GstElement *m_sink = nullptr;

    std::atomic<quint64> m_lastSeq{0};
    QMutex m_frameMutex;
    QImage m_latestFrame;
    quint64 m_latestFrameTime = 0;
    bool m_deliveryScheduled = false;
};
