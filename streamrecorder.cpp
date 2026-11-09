#include "streamrecorder.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QMutexLocker>
#include <QtEndian>
#include <chrono>
#include <limits>

namespace {
qint64 monotonicMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

// Our recorder writes a single video track. stsz stores its finalized sample count.
qint64 mp4Samples(QFile &file, qint64 begin, qint64 end)
{
    for (qint64 offset=begin; offset+8<=end;) {
        if (!file.seek(offset)) return -1;
        const QByteArray header=file.read(8);
        if (header.size()!=8) return -1;
        quint64 size=qFromBigEndian<quint32>(header.constData());
        const QByteArray type=header.mid(4,4);
        qint64 headerSize=8;
        if (size==1) {
            const QByteArray extended=file.read(8);
            if (extended.size()!=8) return -1;
            size=qFromBigEndian<quint64>(extended.constData());
            headerSize=16;
        } else if (size==0) size=end-offset;
        if (size<quint64(headerSize) || size>quint64(end-offset)) return -1;
        if (type=="stsz") {
            const QByteArray table=file.read(12);
            if (table.size()!=12) return -1;
            return qFromBigEndian<quint32>(table.constData()+8);
        }
        if (type=="moov" || type=="trak" || type=="mdia" || type=="minf" || type=="stbl") {
            const qint64 count=mp4Samples(file,offset+headerSize,offset+qint64(size));
            if (count>=0) return count;
        }
        offset+=qint64(size);
    }
    return -1;
}
} // namespace

#include <gst/app/gstappsink.h>
#include <gst/rtp/gstrtpbuffer.h>

StreamRecorder::StreamRecorder(QObject *parent)
    : QObject(parent)
{
    gst_init(nullptr,nullptr);
    m_monitor.setInterval(100);
    connect(&m_monitor,&QTimer::timeout,this,&StreamRecorder::checkPipeline);
}
StreamRecorder::~StreamRecorder()
{
    stop();
}

bool StreamRecorder::checkStreamAvailable(const QString &url,int timeoutMs,
                                          QString *errorMessage)
{
    QString escapedUrl=url;
    escapedUrl.replace('\\',"\\\\");
    escapedUrl.replace('"',"\\\"");
    const QString description=QString(
        "rtspsrc location=\"%1\" protocols=tcp latency=100 ! "
        "rtph264depay ! h264parse ! "
        "appsink name=probeSink sync=false max-buffers=1 drop=true"
    ).arg(escapedUrl);

    GError *parseError=nullptr;
    GstElement *pipeline=gst_parse_launch(description.toUtf8().constData(),&parseError);
    if(!pipeline) {
        if(errorMessage)
            *errorMessage=parseError?QString::fromUtf8(parseError->message):"pipeline creation failed";
        if(parseError) g_error_free(parseError);
        return false;
    }
    if(parseError) g_error_free(parseError);

    GstElement *sink=gst_bin_get_by_name(GST_BIN(pipeline),"probeSink");
    const GstStateChangeReturn stateResult=gst_element_set_state(pipeline,GST_STATE_PLAYING);
    GstSample *sample=nullptr;
    if(sink && stateResult!=GST_STATE_CHANGE_FAILURE) {
        sample=gst_app_sink_try_pull_sample(
            GST_APP_SINK(sink),static_cast<GstClockTime>(timeoutMs)*GST_MSECOND);
    }

    if(!sample && errorMessage) {
        GstBus *bus=gst_element_get_bus(pipeline);
        GstMessage *message=gst_bus_pop_filtered(bus,GST_MESSAGE_ERROR);
        if(message) {
            GError *streamError=nullptr;
            gchar *debugInfo=nullptr;
            gst_message_parse_error(message,&streamError,&debugInfo);
            *errorMessage=streamError?QString::fromUtf8(streamError->message):"stream unavailable";
            if(streamError) g_error_free(streamError);
            g_free(debugInfo);
            gst_message_unref(message);
        } else {
            *errorMessage=QString("no H.264 frames within %1 ms").arg(timeoutMs);
        }
        gst_object_unref(bus);
    }

    const bool available=sample!=nullptr;
    if(sample) gst_sample_unref(sample);
    gst_element_set_state(pipeline,GST_STATE_NULL);
    if(sink) gst_object_unref(sink);
    gst_object_unref(pipeline);
    return available;
}

bool StreamRecorder::start(const QString &url, const QString &sessionBasePath)
{
    if(m_running)
        return false;

    m_failed=false;
    m_receivedEos=false;
    m_frameCountVerified=false;
    m_haveKeyframe=false;
    m_segmentFirstPts=GST_CLOCK_TIME_NONE;
    m_segmentLimitRequested=false;
    m_lastError.clear();
    {
        QMutexLocker locker(&m_mutex);
        m_rtpByPts.clear();
        m_recordedTimestamps.clear();
        m_recordedTelemetry.clear();
        m_pendingEncodedFrames.clear();
    }
    m_lastFrameMs.store(monotonicMs());
    m_rtpProgress.reset();
    m_inputProgress.reset();
    m_decodedProgress.reset();
    m_overlayProgress.reset();
    m_encodedProgress.reset();
    m_nextProgressMs = monotonicMs() + 2000;
    tag=url.split("/").last();
    if (tag.isEmpty())
        tag="stream";

    if (sessionBasePath.isEmpty()) {
        const QString outputDirectory =
            QDir(QCoreApplication::applicationDirPath()).filePath("video-telemetry");
        if (!QDir().mkpath(outputDirectory)) {
            qWarning()<<"Cannot create recording directory:"<<outputDirectory;
            return false;
        }
        m_outputBasePath = QDir(outputDirectory).filePath(
            "stream_"+QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz"));
    } else {
        m_outputBasePath=sessionBasePath;
    }

    m_log.setFileName(m_outputBasePath+"_"+tag+".recording.log");
    if (!m_log.open(QIODevice::WriteOnly|QIODevice::Text|QIODevice::Truncate))
        qWarning()<<"Cannot open recording log:"<<m_log.errorString();
    log("Starting "+tag+" recorder");
    if(!buildPipeline(url)) {
        fail("Cannot create or link recording pipeline (check installed GStreamer plugins)");
        return false;
    }
    if(gst_element_set_state(m_pipeline,GST_STATE_PLAYING)==GST_STATE_CHANGE_FAILURE) {
        fail("Pipeline failed to enter PLAYING");
        destroyPipeline();
        return false;
    }

    m_running=true;
    m_monitor.start();

    return true;
}
void StreamRecorder::log(const QString &message)
{
    const QByteArray line=(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)
                           +" "+message+"\n").toUtf8();
    qInfo().noquote()<<tag<<message;
    if (m_log.isOpen()) {
        m_log.write(line);
        m_log.flush();
    }
}

void StreamRecorder::fail(const QString &message)
{
    log("ERROR: "+message);
    if (m_failed) return;
    m_failed=true;
    m_lastError=tag+": "+message;
    emit recordingError(m_lastError);
}

void StreamRecorder::handleMessage(GstMessage *message)
{
    if (GST_MESSAGE_TYPE(message)==GST_MESSAGE_ERROR
            || GST_MESSAGE_TYPE(message)==GST_MESSAGE_WARNING) {
        GError *error=nullptr;
        gchar *debug=nullptr;
        const bool fatal=GST_MESSAGE_TYPE(message)==GST_MESSAGE_ERROR;
        if (fatal) gst_message_parse_error(message,&error,&debug);
        else gst_message_parse_warning(message,&error,&debug);
        const QString text=QString("%1: %2; %3")
            .arg(QString::fromUtf8(GST_OBJECT_NAME(GST_MESSAGE_SRC(message))))
            .arg(error?QString::fromUtf8(error->message):"unknown GStreamer error")
            .arg(debug?QString::fromUtf8(debug):QString());
        if (fatal) fail(text); else log("WARNING: "+text);
        if (error) g_error_free(error);
        g_free(debug);
    } else if (GST_MESSAGE_TYPE(message)==GST_MESSAGE_EOS) {
        m_receivedEos=true;
        if (m_running) fail("Unexpected end of stream");
    } else if (GST_MESSAGE_TYPE(message)==GST_MESSAGE_ELEMENT) {
        const GstStructure *structure=gst_message_get_structure(message);
        if (structure && gst_structure_has_name(structure,"GstRTSPSrcTimeout"))
            fail("RTSP source timeout");
    }
}

void StreamRecorder::checkPipeline()
{
    if (!m_running || !m_pipeline) return;
    GstBus *bus=gst_element_get_bus(m_pipeline);
    while (GstMessage *message=gst_bus_pop(bus)) {
        handleMessage(message);
        gst_message_unref(message);
    }
    gst_object_unref(bus);
    const qint64 now = monotonicMs();
    if (!m_failed && now - m_lastFrameMs.load() > 5000) {
        logProgress();
        fail("No re-encoded H.264 frames at MP4 input for 5 seconds (see recording stages above)");
    } else if (!m_failed && now >= m_nextProgressMs) {
        logProgress();
        m_nextProgressMs = now + 2000;
    }
}

void StreamRecorder::logProgress()
{
    const qint64 now = monotonicMs();
    const auto describe = [now](const char* name, const Progress& stage) {
        const qint64 last = stage.lastMs.load();
        return QString("%1=%2 age=%3")
            .arg(QString::fromLatin1(name)).arg(stage.count.load())
            .arg(last ? QString::number(now - last) + "ms" : QString("never"));
    };
    // Only atomics: never wait for a streaming-thread mutex during a stall.
    log("STAGES " + describe("rtp_packets", m_rtpProgress) + "; "
        + describe("parsed_buffers", m_inputProgress) + "; "
        + describe("decoded_frames", m_decodedProgress) + "; "
        + describe("overlay_frames", m_overlayProgress) + "; "
        + describe("encoded_frames", m_encodedProgress));
}

QVector<quint64> StreamRecorder::recordedTimestamps() const
{
    QMutexLocker locker(&m_mutex);
    return m_recordedTimestamps;
}

void StreamRecorder::setTelemetryProvider(std::function<QJsonObject(quint64)> provider)
{
    Q_ASSERT(!m_pipeline);
    if (!m_pipeline) m_telemetryProvider=std::move(provider);
}

QVector<QJsonObject> StreamRecorder::recordedTelemetry() const
{
    QMutexLocker locker(&m_mutex);
    return m_recordedTelemetry;
}

void StreamRecorder::stop()
{
    m_monitor.stop();
    if (!m_pipeline) {
        m_running=false;
        if (m_log.isOpen()) m_log.close();
        return;
    }
    // Bound finalization; an ERROR must never cause an infinite EOS wait.
    m_running=false;
    GstBus *bus=gst_element_get_bus(m_pipeline);
    // Inject EOS at the recording branch, not rtspsrc. The RTSP source can
    // reject a pipeline-wide EOS, leaving the MP4 muxer without finalization.
    bool eosAccepted=m_receivedEos;
    if (!m_receivedEos && m_depay) {
        GstPad *sinkPad=gst_element_get_static_pad(m_depay,"sink");
        eosAccepted=sinkPad && gst_pad_send_event(sinkPad,gst_event_new_eos());
        if (sinkPad) gst_object_unref(sinkPad);
        if (!eosAccepted) fail("Recording branch rejected EOS");
    }
    if (!m_receivedEos && eosAccepted) {
        const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
        while (!m_receivedEos && std::chrono::steady_clock::now()<deadline) {
            GstMessage *message=gst_bus_timed_pop(bus,100*GST_MSECOND);
            if (!message) continue;
            const bool fatal=GST_MESSAGE_TYPE(message)==GST_MESSAGE_ERROR;
            handleMessage(message);
            gst_message_unref(message);
            if (fatal) break;
        }
    }
    if (!m_receivedEos && eosAccepted) fail("MP4 finalization did not complete within 5 seconds");
    gst_object_unref(bus);
    destroyPipeline(); // waits until streaming callbacks have finished

    QFile video(m_outputBasePath+"_"+tag+".mp4");
    qint64 count=-1;
    if (video.open(QIODevice::ReadOnly)) count=mp4Samples(video,0,video.size());
    const auto timestamps=recordedTimestamps();
    m_frameCountVerified=count>=0 && count==timestamps.size();
    if (!m_frameCountVerified)
        fail(QString("MP4 sample count %1 differs from tracked frames %2; frame association unverified")
             .arg(count).arg(timestamps.size()));
    // The sidecar must have exactly the finalized MP4 sample count, including
    // explicit unknown entries on a mismatch. Never invent a telemetry timestamp.
    if (count>=0 && count<=std::numeric_limits<int>::max()) {
        QMutexLocker locker(&m_mutex);
        m_recordedTimestamps.resize(int(count));
        if (m_telemetryProvider) m_recordedTelemetry.resize(int(count));
    }
    logProgress();
    log(QString("Stopped: MP4 samples=%1; tracked=%2; verified=%3")
         .arg(count).arg(timestamps.size()).arg(m_frameCountVerified));
    if (m_log.isOpen()) m_log.close();
}

bool StreamRecorder::buildPipeline(const QString &url)
{
    m_pipeline=gst_pipeline_new("stream-recorder");
    m_source=gst_element_factory_make("rtspsrc","src");
    m_depay=gst_element_factory_make("rtph264depay","depay");
    m_parse=gst_element_factory_make("h264parse","input-parser");
    GstElement *decoder=gst_element_factory_make("avdec_h264","decoder");
    GstElement *convert=gst_element_factory_make("videoconvert","overlay-convert");
    m_overlay=gst_element_factory_make("textoverlay","timestamp-overlay");
    GstElement *encoderConvert=gst_element_factory_make("videoconvert","encoder-convert");
    GstElement *rawCaps=gst_element_factory_make("capsfilter","encoder-input");
    GstElement *encoder=gst_element_factory_make("x264enc","encoder");
    GstElement *parser=gst_element_factory_make("h264parse","output-parser");
    GstElement *capsfilter=gst_element_factory_make("capsfilter","access-units");
    m_sink=gst_element_factory_make("splitmuxsink","sink");
    const QList<GstElement*> elements={m_source,m_depay,m_parse,decoder,convert,m_overlay,
        encoderConvert,rawCaps,encoder,parser,capsfilter,m_sink};
    if (!m_pipeline || elements.contains(nullptr)) {
        qWarning()<<"Missing recording plugin: avdec_h264, textoverlay or x264enc (or base elements)";
        for (GstElement *element : elements) if (element) gst_object_unref(element);
        m_source=m_depay=m_parse=m_overlay=m_sink=nullptr;
        destroyPipeline();
        return false;
    }
    GstCaps *caps=gst_caps_from_string("video/x-h264,stream-format=avc,alignment=au");
    g_object_set(capsfilter,"caps",caps,nullptr);
    gst_caps_unref(caps);
    caps=gst_caps_from_string("video/x-raw,format=I420");
    g_object_set(rawCaps,"caps",caps,nullptr);
    gst_caps_unref(caps);
    g_object_set(m_overlay,"text","TS: waiting","font-desc","Sans 20",
                 "shaded-background",TRUE,"auto-resize",FALSE,"xpad",16,"ypad",16,nullptr);
    // Keep the frame probe for TS/telemetry matching; suppress visible text.
    g_object_set(m_overlay,"silent",TRUE,nullptr);
    gst_util_set_object_arg(G_OBJECT(m_overlay),"halignment","left");
    gst_util_set_object_arg(G_OBJECT(m_overlay),"valignment","top");
    gst_util_set_object_arg(G_OBJECT(encoder),"tune","zerolatency");
    gst_util_set_object_arg(G_OBJECT(encoder),"speed-preset","ultrafast");
    g_object_set(encoder,"bitrate",tag=="rgb"?9000:6000,"key-int-max",30,"bframes",0,nullptr);
    const QString filename=m_outputBasePath+"_"+tag+".mp4";
    log("Recording without visible RTP TS (software H.264 encoding)");
    g_object_set(m_source,"location",url.toUtf8().constData(),"protocols",4,"latency",100,nullptr);
    g_object_set(m_sink,"location",filename.toUtf8().constData(),nullptr);
    for (GstElement *element : elements) gst_bin_add(GST_BIN(m_pipeline),element);
    if (!gst_element_link_many(m_depay,m_parse,decoder,convert,m_overlay,encoderConvert,
                               rawCaps,encoder,parser,capsfilter,m_sink,nullptr)) {
        qWarning()<<"Cannot link timestamp recording pipeline";
        destroyPipeline();
        return false;
    }
    GstPad *inputPad = gst_element_get_static_pad(m_parse, "src");
    gst_pad_add_probe(inputPad, GST_PAD_PROBE_TYPE_BUFFER, onInputProbe, this, nullptr);
    gst_object_unref(inputPad);
    GstPad *decodedPad = gst_element_get_static_pad(decoder, "src");
    gst_pad_add_probe(decodedPad, GST_PAD_PROBE_TYPE_BUFFER, onDecodedProbe, this, nullptr);
    gst_object_unref(decodedPad);
    GstPad *overlayPad=gst_element_get_static_pad(m_overlay,"video_sink");
    gst_pad_add_probe(overlayPad,GST_PAD_PROBE_TYPE_BUFFER,onOverlayProbe,this,nullptr);
    gst_object_unref(overlayPad);
    GstPad *framePad=gst_element_get_static_pad(capsfilter,"src");
    gst_pad_add_probe(framePad,GST_PAD_PROBE_TYPE_BUFFER,onFrameProbe,this,nullptr);
    gst_object_unref(framePad);
    g_signal_connect(m_source,"pad-added",G_CALLBACK(StreamRecorder::onPadAdded),this);
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
    m_overlay=nullptr;
    m_sink=nullptr;
}

void StreamRecorder::onPadAdded( GstElement *,GstPad *pad, gpointer data){
    StreamRecorder *self = static_cast<StreamRecorder*>(data);
    GstCaps *caps = gst_pad_get_current_caps(pad);
    if(!caps)  return;
    GstStructure *structure = gst_caps_get_structure(caps,0);
    QString name = gst_structure_get_name(structure);
    const QString media = QString::fromUtf8(gst_structure_get_string(structure,"media"));
    const QString encoding = QString::fromUtf8(gst_structure_get_string(structure,"encoding-name"));
    gst_caps_unref(caps);
    if(!name.startsWith("application/x-rtp") || media!="video" || encoding!="H264") return;
    gst_pad_add_probe(pad,GST_PAD_PROBE_TYPE_BUFFER,
                      StreamRecorder::onRtpProbe,self,nullptr);
    GstPad *sinkpad = gst_element_get_static_pad( self->m_depay, "sink");
    if(!gst_pad_is_linked(sinkpad)){
        GstPadLinkReturn ret = gst_pad_link(pad, sinkpad);
        if (ret!=GST_PAD_LINK_OK) {
            const QString error=QString("RTSP pad link failed: %1").arg(int(ret));
            QMetaObject::invokeMethod(self,[self,error]() { self->fail(error); },Qt::QueuedConnection);
        }
    }
    gst_object_unref(sinkpad);
}

GstPadProbeReturn StreamRecorder::onRtpProbe(GstPad *,GstPadProbeInfo *info,gpointer data)
{
    StreamRecorder *self=static_cast<StreamRecorder*>(data);
    GstBuffer *buffer=gst_pad_probe_info_get_buffer(info);
    if(!buffer) return GST_PAD_PROBE_OK;

    GstRTPBuffer rtp=GST_RTP_BUFFER_INIT;
    if(!gst_rtp_buffer_map(buffer,GST_MAP_READ,&rtp))
        return GST_PAD_PROBE_OK;
    self->m_rtpProgress.count.fetch_add(1);
    self->m_rtpProgress.lastMs.store(monotonicMs());
    const bool marker=gst_rtp_buffer_get_marker(&rtp);
    const quint64 timestamp=gst_rtp_buffer_get_timestamp(&rtp);
    gst_rtp_buffer_unmap(&rtp);

    if(marker && GST_CLOCK_TIME_IS_VALID(GST_BUFFER_PTS(buffer))) {
        QMutexLocker locker(&self->m_mutex);
        self->m_rtpByPts.insert(GST_BUFFER_PTS(buffer),timestamp);
        while (self->m_rtpByPts.size()>1024)
            self->m_rtpByPts.erase(self->m_rtpByPts.begin());
    }
    return GST_PAD_PROBE_OK;
}

GstPadProbeReturn StreamRecorder::onInputProbe(GstPad *, GstPadProbeInfo *info, gpointer data)
{
    auto* self = static_cast<StreamRecorder*>(data);
    GstBuffer* buffer = gst_pad_probe_info_get_buffer(info);
    if (buffer && gst_buffer_get_size(buffer)) {
        self->m_inputProgress.count.fetch_add(1);
        self->m_inputProgress.lastMs.store(monotonicMs());
    }
    return GST_PAD_PROBE_OK;
}

GstPadProbeReturn StreamRecorder::onDecodedProbe(GstPad *pad, GstPadProbeInfo *info, gpointer data)
{
    auto* self = static_cast<StreamRecorder*>(data);
    if (!gst_pad_probe_info_get_buffer(info)) return GST_PAD_PROBE_OK;
    const bool first = self->m_decodedProgress.count.fetch_add(1) == 0;
    self->m_decodedProgress.lastMs.store(monotonicMs());
    if (first) {
        GstCaps* caps = gst_pad_get_current_caps(pad);
        if (caps) {
            gchar* value = gst_caps_to_string(caps);
            const QString text = QString("Decoded video caps: ") + QString::fromUtf8(value);
            g_free(value);
            gst_caps_unref(caps);
            QMetaObject::invokeMethod(self, [self, text]() { self->log(text); }, Qt::QueuedConnection);
        }
    }
    return GST_PAD_PROBE_OK;
}

GstPadProbeReturn StreamRecorder::onOverlayProbe(GstPad *,GstPadProbeInfo *info,gpointer data)
{
    auto *self=static_cast<StreamRecorder*>(data);
    GstBuffer *buffer=gst_pad_probe_info_get_buffer(info);
    if (!buffer) return GST_PAD_PROBE_OK;
    self->m_overlayProgress.count.fetch_add(1);
    self->m_overlayProgress.lastMs.store(monotonicMs());
    quint64 timestamp=0;
    {
        QMutexLocker locker(&self->m_mutex);
        auto it=self->m_rtpByPts.find(GST_BUFFER_PTS(buffer));
        if (it!=self->m_rtpByPts.end()) {
            timestamp=it.value();
            self->m_rtpByPts.erase(it);
        }
    }
    const QByteArray text=(self->tag.toUpper()+" TS: "
                          +(timestamp?QString::number(timestamp):QString("unavailable"))).toUtf8();
    g_object_set(self->m_overlay,"text",text.constData(),nullptr);
    const QJsonObject telemetry=self->m_telemetryProvider
        ? self->m_telemetryProvider(timestamp) : QJsonObject();
    {
        QMutexLocker locker(&self->m_mutex);
        // zerolatency + bframes=0 preserves this raw-frame order. Do not use
        // encoder PTS as x264 adds its own timestamp offset.
        self->m_pendingEncodedFrames.enqueue({timestamp,telemetry});
    }
    return GST_PAD_PROBE_OK;
}

GstPadProbeReturn StreamRecorder::onFrameProbe(GstPad *,GstPadProbeInfo *info,gpointer data)
{
    auto *self=static_cast<StreamRecorder*>(data);
    GstBuffer *buffer=gst_pad_probe_info_get_buffer(info);
    if (!buffer || gst_buffer_get_size(buffer)==0) return GST_PAD_PROBE_OK;
    const GstClockTime pts = GST_BUFFER_PTS(buffer);
    if (!GST_CLOCK_TIME_IS_VALID(self->m_segmentFirstPts)) self->m_segmentFirstPts = pts;
    constexpr GstClockTime limit = 600 * GST_SECOND;
    const bool limitReached = !GST_CLOCK_TIME_IS_VALID(pts)
            || pts < self->m_segmentFirstPts || pts - self->m_segmentFirstPts >= limit;
    if (limitReached && !self->m_segmentLimitRequested.exchange(true))
        emit self->segmentLimitReached();
    if (!limitReached && GST_CLOCK_TIME_IS_VALID(GST_BUFFER_DURATION(buffer))) {
        buffer = gst_buffer_make_writable(buffer);
        GST_PAD_PROBE_INFO_DATA(info) = buffer;
        GST_BUFFER_DURATION(buffer) = qMin(GST_BUFFER_DURATION(buffer),
                                           limit - (pts - self->m_segmentFirstPts));
    }
    self->m_encodedProgress.count.fetch_add(1);
    self->m_encodedProgress.lastMs.store(monotonicMs());
    quint64 timestamp=0;
    {
        QMutexLocker locker(&self->m_mutex);
        if (self->m_pendingEncodedFrames.isEmpty()) {
            QMetaObject::invokeMethod(self,[self]() {
                self->fail("Encoded frame has no matching TS/telemetry snapshot");
            },Qt::QueuedConnection);
            return GST_PAD_PROBE_DROP;
        }
        const auto frame=self->m_pendingEncodedFrames.dequeue();
        if (self->m_segmentLimitRequested.load()) return GST_PAD_PROBE_DROP;
        timestamp=frame.timestamp;
        self->m_recordedTimestamps.append(timestamp);
        if (self->m_telemetryProvider) self->m_recordedTelemetry.append(frame.telemetry);
    }
    self->m_lastFrameMs.store(monotonicMs());
    emit self->frameRecorded(timestamp);
    return GST_PAD_PROBE_OK;
}
