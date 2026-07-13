#include "RtspReceiver.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>

#include <gst/rtp/gstrtpbuffer.h>
#include <gst/video/video.h>

RtspReceiver::RtspReceiver(QObject *parent)
    : QThread(parent)
{
   auto base = QCoreApplication::applicationDirPath(); //for release
   QString gstPath = base + "/gstreamer-1.0";
   if (QDir(gstPath).exists()){
       qputenv("PATH", (base + ";" + base + "/gstreamer-1.0").toUtf8());

       qputenv("GST_PLUGIN_PATH", (base + "/gstreamer-1.0").toUtf8());
       qputenv("GST_PLUGIN_SYSTEM_PATH", (base + "/gstreamer-1.0").toUtf8());
       qputenv("GST_REGISTRY_FORK", "no"); // важно для Windows debug
   }
   gst_init(nullptr, nullptr);
}


RtspReceiver::~RtspReceiver()
{
    stop();
    wait();
}

void RtspReceiver::setUrl(const QString &url)
{
    m_url = url;
}

void RtspReceiver::stop()
{
    m_running = false;

    if (m_pipeline) {
        gst_element_send_event(m_pipeline, gst_event_new_eos());
    }
}

void RtspReceiver::run()
{
    m_running = true;

    QString pipelineDesc =
        QString(
            "rtspsrc name=src location=\"%1\" protocols=tcp latency=0 ! "
            "rtph264depay ! "
            "h264parse ! "
            "avdec_h264 ! "
            "videoconvert ! "
            "video/x-raw,format=RGB ! "
            "appsink name=sink sync=false emit-signals=true max-buffers=1 drop=true"
            ).arg(m_url);

//    QString pipelineDesc =
//        QString(
//            "rtspsrc name=src location=\"%1\" protocols=tcp latency=0 ! "
//            "rtph265depay ! "
//            "h265parse ! "
//            "avdec_h265 ! "
//            "videoconvert ! "
//            "video/x-raw,format=RGB ! "
//            "appsink name=sink "
//            "sync=false emit-signals=true max-buffers=1 drop=true"
//        ).arg(m_url);

    GError *err = nullptr;

    m_pipeline = gst_parse_launch(pipelineDesc.toUtf8().constData(), &err);

    if (!m_pipeline) {
        qDebug() << "Pipeline create failed:" << (err ? err->message : "");
        return;
    }

    GstElement *src = gst_bin_get_by_name(GST_BIN(m_pipeline), "src");
    m_sink = gst_bin_get_by_name(GST_BIN(m_pipeline), "sink");

    g_signal_connect(src, "new-manager", G_CALLBACK(on_new_manager), this);
    g_signal_connect(m_sink, "new-sample", G_CALLBACK(on_new_sample), this);
    g_signal_connect(src, "pad-added", G_CALLBACK(on_pad_added), this);

    gst_element_set_state(m_pipeline, GST_STATE_PLAYING);

    emit connected();

    GstBus *bus = gst_element_get_bus(m_pipeline);

    while (m_running) {
        GstMessage *msg = gst_bus_timed_pop_filtered(
            bus,
            100 * GST_MSECOND,
            (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS)
            );

        if (msg) {
            switch (GST_MESSAGE_TYPE(msg)) {
            case GST_MESSAGE_ERROR:
                qDebug() << "GStreamer ERROR";
                m_running = false;
                break;

            case GST_MESSAGE_EOS:
                m_running = false;
                break;

            default:
                break;
            }

            gst_message_unref(msg);
        }
    }

    emit disconnected();

    gst_object_unref(bus);

    gst_element_set_state(m_pipeline, GST_STATE_NULL);

    gst_object_unref(src);
    gst_object_unref(m_sink);
    gst_object_unref(m_pipeline);

    m_pipeline = nullptr;
    m_sink = nullptr;
}


GstFlowReturn RtspReceiver::on_new_sample(GstAppSink *sink, gpointer user_data)
{
    RtspReceiver *self = static_cast<RtspReceiver*>(user_data);

    GstSample *sample = gst_app_sink_pull_sample(sink);
    if (!sample)
        return GST_FLOW_OK;

    GstBuffer *buffer = gst_sample_get_buffer(sample);
    GstCaps *caps = gst_sample_get_caps(sample);

    GstStructure *s = gst_caps_get_structure(caps, 0);

    int width = 0;
    int height = 0;

    gst_structure_get_int(s, "width", &width);
    gst_structure_get_int(s, "height", &height);

    GstMapInfo map;

    if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        QImage img(
            map.data,
            width,
            height,
            width * 3,
            QImage::Format_RGB888
            );

        emit self->frameReady(self->m_lastSeq, img.copy());
//        qDebug()<<"frameFrame "<<self->m_lastSeq<<img.size();
        gst_buffer_unmap(buffer, &map);
    }

    gst_sample_unref(sample);

    return GST_FLOW_OK;
}

void RtspReceiver::on_new_manager(
    GstElement *rtspsrc,
    GstElement *manager,
    gpointer user_data)
{
    RtspReceiver *self = static_cast<RtspReceiver*>(user_data);

    qDebug() << "RTSP manager created";

    self->m_manager = GST_ELEMENT(gst_object_ref(manager));
}

void RtspReceiver::on_pad_added(
    GstElement *src,
    GstPad *pad,
    gpointer user_data)
{
    RtspReceiver *self = static_cast<RtspReceiver*>(user_data);

    qDebug() << "pad-added";

    gst_pad_add_probe(
        pad,
        GST_PAD_PROBE_TYPE_BUFFER,
        on_rtp_probe,
        self,
        nullptr
    );

    GstElement *depay = gst_bin_get_by_name(GST_BIN(self->m_pipeline), "depay");
    GstPad *sinkpad = gst_element_get_static_pad(depay, "sink");

    if (!gst_pad_is_linked(sinkpad))
        gst_pad_link(pad, sinkpad);

    gst_object_unref(sinkpad);
    gst_object_unref(depay);
}



GstPadProbeReturn RtspReceiver::on_rtp_probe(
    GstPad *pad,
    GstPadProbeInfo *info,
    gpointer user_data)
{
    RtspReceiver *self = static_cast<RtspReceiver*>(user_data);

    GstBuffer *buffer = gst_pad_probe_info_get_buffer(info);
    if (!buffer)
        return GST_PAD_PROBE_OK;
    GstClockTime pts = GST_BUFFER_PTS(buffer);

    GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;

    if (!gst_rtp_buffer_map(buffer, GST_MAP_READ, &rtp))
        return GST_PAD_PROBE_OK;

    guint16 seq = gst_rtp_buffer_get_seq(&rtp);
    quint64 ts  = gst_rtp_buffer_get_timestamp(&rtp);
    quint32 ssrc = gst_rtp_buffer_get_ssrc(&rtp);
    gboolean marker = gst_rtp_buffer_get_marker(&rtp);


    gst_rtp_buffer_unmap(&rtp);
    self->m_lastSeq = ts;

//    if(marker)qDebug() << "RTP" << "seq =" << seq<< "ts =" << ts << "ssrc =" << ssrc<< "marker =" << marker<<gst_rtp_buffer_get_header_len(&rtp);

    return GST_PAD_PROBE_OK;
}
