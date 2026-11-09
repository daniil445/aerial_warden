#include "VideoRecorder.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QWidget>

VideoRecorder::VideoRecorder(QObject *parent):QObject(parent)
{
}

VideoRecorder::~VideoRecorder()
{
    stop();
}
bool VideoRecorder::buildPipeline(int w,int h,int fps) {
    const QString outputDirectory =
        QDir(QCoreApplication::applicationDirPath()).filePath("video-telemetry");
    if (!QDir().mkpath(outputDirectory)) {
        qWarning() << "Cannot create recording directory:" << outputDirectory;
        return false;
    }

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
    if (m_outputBasePath.isEmpty())
        m_outputBasePath = QDir(outputDirectory).filePath(
            "record_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz"));
    const QString filename = m_outputBasePath + ".mp4";
    g_object_set(m_sink, "location",filename.toUtf8().constData(), nullptr);
    GstCaps *caps=gst_caps_new_simple(
        "video/x-raw", "format",G_TYPE_STRING,"RGB", "width",G_TYPE_INT,m_width, "height",G_TYPE_INT,m_height,
        // Frames carry source-derived PTS, so the stream is variable-rate.
        "framerate",GST_TYPE_FRACTION,0,1, nullptr);
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
    if (gst_element_set_state(m_pipeline,GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        qWarning() << "Cannot start interface recording pipeline:" << filename;
        destroyPipeline();
        return false;
    }
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

bool VideoRecorder::start(QSize s,int fps,const QString &basePath)
{
    if(m_running)
        return false;
    m_outputBasePath = basePath;
    s.setWidth(s.width() & ~1);
    s.setHeight(s.height() & ~1);
    if(!buildPipeline(s.width(),s.height(),fps)) return false;
    m_running=true;
    m_frame=0;
    m_haveSourceTimestamp=false;
    m_firstSourceTimestamp=0;
    m_previousSourceTimestamp=0;
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
        auto placeholder = gst_bus_timed_pop_filtered(bus, 5 * GST_SECOND,
            GstMessageType(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
        if (!placeholder) qWarning()<<"Interface recorder finalization timeout";
        else gst_message_unref(placeholder);
        gst_object_unref(bus);
    }

    destroyPipeline();
}


bool VideoRecorder::writeFrame(const QImage &src, quint64 sourceTimestamp)
{
    if(!m_running||!m_appsrc||src.isNull()) return false;
    QImage img=src;

    if(img.format()!=QImage::Format_RGB888)img=img.convertToFormat(QImage::Format_RGB888);
    if(img.size()!=QSize(m_width,m_height)){qDebug()<<"Wrong frame size"<<img.size(); return false;}
    GstBuffer *buffer=gst_buffer_new_allocate(nullptr,img.width()*img.height()*3,nullptr);
    GstMapInfo map;
    if(!gst_buffer_map(buffer,&map,GST_MAP_WRITE)){gst_buffer_unref(buffer); return false;}

    for(int y=0;y<img.height();y++){
        memcpy( map.data + y*img.width()*3,img.constScanLine(y), img.width()*3);
    }
    gst_buffer_unmap(buffer,&map);
    constexpr quint64 sourceTimestampRate = 100000;
    GstClockTime pts = 0;
    GstClockTime duration = gst_util_uint64_scale(1, GST_SECOND, m_fps);
    if (!m_haveSourceTimestamp) {
        m_firstSourceTimestamp = sourceTimestamp;
        m_previousSourceTimestamp = sourceTimestamp;
        m_haveSourceTimestamp = true;
    } else if (sourceTimestamp > m_previousSourceTimestamp) {
        duration = gst_util_uint64_scale(sourceTimestamp - m_previousSourceTimestamp,
                                         GST_SECOND, sourceTimestampRate);
        m_previousSourceTimestamp = sourceTimestamp;
    }
    if (sourceTimestamp >= m_firstSourceTimestamp) {
        pts = gst_util_uint64_scale(sourceTimestamp - m_firstSourceTimestamp,
                                    GST_SECOND, sourceTimestampRate);
    } else {
        // RTP timestamp wrap or discontinuity: preserve monotonic output.
        pts = gst_util_uint64_scale(m_frame, GST_SECOND, m_fps);
    }
    GST_BUFFER_PTS(buffer)=pts;
    GST_BUFFER_DTS(buffer)=pts;
    constexpr GstClockTime limit = 600 * GST_SECOND;
    if (pts >= limit) { gst_buffer_unref(buffer); return false; }
    GST_BUFFER_DURATION(buffer)=qMin(duration, limit - pts);
    GstFlowReturn ret=gst_app_src_push_buffer(GST_APP_SRC(m_appsrc),buffer);
    if(ret!=GST_FLOW_OK)
    {
        qDebug()<<"Push error"<<ret;
        stop();
        return false;
    }
    ++m_frame;
    return true;
}
