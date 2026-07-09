#include "VideoRecorder.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QWidget>

VideoRecorder::VideoRecorder(QObject *parent):QObject(parent)
{
}

VideoRecorder::~VideoRecorder()
{
    stop();
}
bool VideoRecorder::buildPipeline(int w,int h,int fps) {
    m_width=w;
    m_height=h;
    m_fps=fps;
    m_pipeline=gst_pipeline_new("recorder");
    m_appsrc=gst_element_factory_make("appsrc","src");
    m_convert=gst_element_factory_make("videoconvert","convert");
    m_encoder=gst_element_factory_make("x264enc","encoder");
    m_mux=gst_element_factory_make("mp4mux","mux");
    m_sink=gst_element_factory_make("filesink","sink");

    if(!m_pipeline||!m_appsrc||!m_convert||!m_encoder||!m_mux||!m_sink) {
        qDebug()<<"Failed create gstreamer elements"; return false;
    }
    QString filename= QCoreApplication::applicationDirPath()+"/record_"+
                      QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")+ ".mp4";
    g_object_set(m_sink, "location",filename.toUtf8().constData(), nullptr);
    GstCaps *caps=gst_caps_new_simple(
        "video/x-raw", "format",G_TYPE_STRING,"RGB", "width",G_TYPE_INT,m_width, "height",G_TYPE_INT,m_height,
        "framerate",GST_TYPE_FRACTION,m_fps,1, nullptr);
    g_object_set(m_appsrc, "caps",caps, "format",GST_FORMAT_TIME,
                           "stream-type",GST_APP_STREAM_TYPE_STREAM, "is-live",TRUE, "block",TRUE, nullptr);
    gst_caps_unref(caps);
    g_object_set(m_encoder, "bitrate",8000, "key-int-max",30, "speed-preset",1, "tune",4, nullptr);
    gst_bin_add_many(GST_BIN(m_pipeline), m_appsrc, m_convert, m_encoder, m_mux, m_sink, nullptr);
    if(!gst_element_link_many( m_appsrc, m_convert, m_encoder, m_mux, m_sink, nullptr)) {
        qDebug()<<"Pipeline link failed";
        destroyPipeline();
        return false;
    }
    gst_element_set_state(m_pipeline,GST_STATE_PLAYING);
    return true;
}
void VideoRecorder::destroyPipeline(){
    if(!m_pipeline) return;
    gst_element_set_state(m_pipeline,GST_STATE_NULL);
    gst_object_unref(m_pipeline);
    m_pipeline=nullptr;
    m_appsrc=nullptr;
    m_convert=nullptr;
    m_encoder=nullptr;
    m_mux=nullptr;
    m_sink=nullptr;
}

bool VideoRecorder::start(QSize s,int fps)
{
    if(m_running)
        return false;
    s.setWidth(s.width() & ~1);
    s.setHeight(s.height() & ~1);
    if(!buildPipeline(s.width(),s.height(),fps)) return false;
    m_running=true;
    m_frame=0;
    return true;
}

void VideoRecorder::stop()
{
    if(!m_running)
        return;
    m_running=false;
    if(m_appsrc) gst_app_src_end_of_stream(GST_APP_SRC(m_appsrc));
    if(m_pipeline)
    {
        GstBus *bus=gst_element_get_bus(m_pipeline);
        auto placeholder = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_EOS);
        qDebug()<<"gst_bus_timed_pop_filtered"<<placeholder;
        gst_object_unref(bus);
    }

    destroyPipeline();
}


void VideoRecorder::writeFrame(const QImage &src)
{
    if(!m_running||!m_appsrc||src.isNull()) return;
    QImage img=src;

    if(img.format()!=QImage::Format_RGB888)img=img.convertToFormat(QImage::Format_RGB888);
    if(img.size()!=QSize(m_width,m_height)){qDebug()<<"Wrong frame size"<<img.size(); return;}
    GstBuffer *buffer=gst_buffer_new_allocate(nullptr,img.width()*img.height()*3,nullptr);
    GstMapInfo map;
    if(!gst_buffer_map(buffer,&map,GST_MAP_WRITE)){gst_buffer_unref(buffer); return;}

    for(int y=0;y<img.height();y++){
        memcpy( map.data + y*img.width()*3,img.constScanLine(y), img.width()*3);
    }
    gst_buffer_unmap(buffer,&map);
    GST_BUFFER_PTS(buffer)=gst_util_uint64_scale(m_frame,GST_SECOND,m_fps);
    GST_BUFFER_DURATION(buffer)=gst_util_uint64_scale(1,GST_SECOND,m_fps);
    GstFlowReturn ret=gst_app_src_push_buffer(GST_APP_SRC(m_appsrc),buffer);
    if(ret!=GST_FLOW_OK)
    {
        qDebug()<<"Push error"<<ret;
        stop();
        return;
    }
    ++m_frame;
}
