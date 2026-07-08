#pragma once

#include <QObject>
#include <QWidget>
#include <QTimer>
#include <QMutex>

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>

class VideoRecorder : public QObject
{
    Q_OBJECT

public:
    explicit VideoRecorder(QObject *parent=nullptr);
    ~VideoRecorder();

    bool start(QWidget *window,int fps=30);
    void stop();

    bool isRecording() const{return m_running;}

private slots:
    void grabFrame();
public slots:
    void addFrame(const QImage&);
private:
    bool buildPipeline(int w,int h,int fps);
    void destroyPipeline();

private:
    QWidget *m_window=nullptr;
    QTimer m_timer;

    QMutex m_frameMutex;
    QImage m_currentFrame;

    GstElement *m_pipeline=nullptr;
    GstElement *m_appsrc=nullptr;
    GstElement *m_convert=nullptr;
    GstElement *m_encoder=nullptr;
    GstElement *m_mux=nullptr;
    GstElement *m_sink=nullptr;

    bool m_running=false;

    quint64 m_frame=0;
    int m_width=0;
    int m_height=0;
    int m_fps=30;
};
