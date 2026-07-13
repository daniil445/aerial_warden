#pragma once

#include <QObject>
#include <QString>
#include <QDebug>
#include <gst/gst.h>

class StreamRecorder : public QObject
{
    Q_OBJECT

public:
    explicit StreamRecorder(QObject *parent=nullptr);
    ~StreamRecorder();

    bool start(const QString &url);
    void stop();

    bool isRecording() const { return m_running; }

private:
    bool buildPipeline(const QString &url);
    void destroyPipeline();

    static void onPadAdded(GstElement *src,GstPad *pad,gpointer data);

private:
    bool m_running=false;

    GstElement *m_pipeline=nullptr;

    GstElement *m_source=nullptr;
    GstElement *m_depay=nullptr;
    GstElement *m_parse=nullptr;
    GstElement *m_sink=nullptr;
};
