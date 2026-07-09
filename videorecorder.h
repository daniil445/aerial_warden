#pragma once

#include <QObject>
#include <QImage>

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>

class VideoRecorder : public QObject
{
    Q_OBJECT

public:
    explicit VideoRecorder(QObject *parent=nullptr);
    ~VideoRecorder();

    bool start(QSize ,int fps=30);
    void stop();
    bool isRecording() const{return m_running;}
    void writeFrame(const QImage &frame);

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
};
