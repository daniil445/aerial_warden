#include "streamrecorder.h"

#include <QCoreApplication>
#include <QDateTime>

StreamRecorder::StreamRecorder(QObject *parent)
    : QObject(parent)
{
    gst_init(nullptr,nullptr);
}
StreamRecorder::~StreamRecorder()
{
    stop();
}
bool StreamRecorder::start(const QString &url)
{
    if(m_running)
        return false;

    if(!buildPipeline(url))
        return false;

    gst_element_set_state(
        m_pipeline,
        GST_STATE_PLAYING);

    m_running=true;

    return true;
}
void StreamRecorder::stop()
{
    if(!m_running)
        return;

    gst_element_send_event(
        m_pipeline,
        gst_event_new_eos());

    GstBus *bus=
        gst_element_get_bus(m_pipeline);

    gst_bus_timed_pop_filtered(
        bus,
        GST_CLOCK_TIME_NONE,
        GST_MESSAGE_EOS);

    gst_object_unref(bus);

    destroyPipeline();

    m_running=false;
}

bool StreamRecorder::buildPipeline(const QString &url)
{
    m_pipeline = gst_pipeline_new("stream-recorder");
    m_source = gst_element_factory_make("rtspsrc", "src");
    m_depay = gst_element_factory_make("rtph264depay", "depay");
    m_parse = gst_element_factory_make("h264parse", "parse");
    m_sink = gst_element_factory_make("splitmuxsink", "sink");
    if(!m_pipeline || !m_source || !m_depay || !m_parse || !m_sink){
        qDebug()<<"Failed create recorder elements";
        destroyPipeline();
        return false;
    }

    QString filename = QCoreApplication::applicationDirPath() + "/camera_"
       + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".mp4";
    qDebug()<<"Recording:"<<filename;
    g_object_set( m_source, "location", url.toUtf8().constData(), "protocols", 4, "latency", 0, nullptr);
    g_object_set( m_sink, "location", filename.toUtf8().constData(), nullptr);
    gst_bin_add_many(GST_BIN(m_pipeline), m_source, m_depay, m_parse, m_sink, nullptr);
    if(!gst_element_link(m_depay, m_parse)){
        qDebug()<<"depay -> parse link failed";
        destroyPipeline();
        return false;
    }
    if(!gst_element_link(m_parse, m_sink)){
        qDebug()<<"parse -> sink link failed";
        destroyPipeline();
        return false;
    }
    g_signal_connect(m_source, "pad-added",G_CALLBACK(StreamRecorder::onPadAdded),this);
    return true;
}

void StreamRecorder::destroyPipeline()
{
    if(!m_pipeline) return;
    gst_element_set_state(m_pipeline,GST_STATE_NULL);
    gst_object_unref(m_pipeline);
    m_pipeline=nullptr;
    m_source=nullptr;
    m_depay=nullptr;
    m_parse=nullptr;
    m_sink=nullptr;
}

void StreamRecorder::onPadAdded( GstElement *,GstPad *pad, gpointer data){
    StreamRecorder *self = static_cast<StreamRecorder*>(data);
    GstCaps *caps = gst_pad_get_current_caps(pad);
    if(!caps)  return;
    QString name = gst_structure_get_name( gst_caps_get_structure(caps,0));
    gst_caps_unref(caps);
    if(!name.startsWith("application/x-rtp")) return;
    GstPad *sinkpad = gst_element_get_static_pad( self->m_depay, "sink");
    if(!gst_pad_is_linked(sinkpad)){
        GstPadLinkReturn ret = gst_pad_link(pad, sinkpad);
        qDebug()<<"RTSP pad link"<<ret;
    }
    gst_object_unref(sinkpad);
}
