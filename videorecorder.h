#pragma once

#include <QObject>
#include <QImage>
#include <QString>

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>

class VideoRecorder : public QObject
{
    Q_OBJECT

public:
    explicit VideoRecorder(QObject *parent=nullptr);
    ~VideoRecorder();

    bool start(QSize ,int fps=30, const QString &basePath=QString());
    void stop();
    bool isRecording() const{return m_running;}
    quint64 frameCount() const{return m_frame;}
    QString outputBasePath() const{return m_outputBasePath;}
    bool writeFrame(const QImage &frame, quint64 sourceTimestamp);

private:
    bool buildPipeline(int w,int h,int fps);
    void destroyPipeline();

private:
    GstElement *m_pipeline=nullptr;
    GstElement *m_appsrc=nullptr;
    GstElement *m_convert=nullptr;
    GstElement *m_encoder=nullptr;
    GstElement *m_mux=nullptr;
    GstElement *m_sink=nullptr;

    bool m_running=false;

    int m_width=0;
    int m_height=0;
    int m_fps=30;

    quint64 m_frame=0;
    quint64 m_firstSourceTimestamp=0;
    quint64 m_previousSourceTimestamp=0;
    bool m_haveSourceTimestamp=false;
    QString m_outputBasePath;
};
