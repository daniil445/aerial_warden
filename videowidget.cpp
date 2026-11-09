#include "videowidget.h"
#include <QJsonArray>
#include <QJsonObject>

#include <QMouseEvent>
#include <QSettings>
#include <QDebug>
#include <QDateTime>
#include <QApplication>
#include <QWheelEvent>
#include <QFocusEvent>
#include <QHideEvent>
#include <algorithm>
#include <cmath>

VideoWidget::VideoWidget(QWidget *parent)
    : QOpenGLWidget(parent)
{
    m_precisionIcon.load(":/img/image/icons/precision_magnifier_generated.png");
    QSettings settings("config.ini", QSettings::IniFormat);
    m_objectNames = settings.value("obj_name").toStringList();
    connect(qApp, &QGuiApplication::applicationStateChanged, this,
            [this](Qt::ApplicationState state) {
        if (state != Qt::ApplicationActive) stopMouseSteering();
    });
}

QJsonObject VideoWidget::recordingTelemetrySnapshot(quint64 videoTimestamp) const
{
    QMutexLocker locker(&m_recordingTelemetryMutex);
    QJsonObject result=m_recordingTelemetry;
    QJsonObject snapshot;
    snapshot.insert("source","VideoWidget");
    snapshot.insert("camera",m_recordingCamera);
    snapshot.insert("available",!m_recordingTelemetry.isEmpty());
    snapshot.insert("video_timestamp",qint64(videoTimestamp));
    snapshot.insert("captured_at_utc",QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    result.insert("_video_widget",snapshot);
    return result;
}

QImage VideoWidget::renderFrameForRecording(const QImage &frame)
{
    if (frame.isNull() || size().isEmpty())
        return {};

    QImage result(size(), QImage::Format_RGB888);
    result.fill(Qt::black);
    QPainter painter(&result);
    painter.drawImage(result.rect(), frame);
    paint_overlay(&painter);
    if (m_storage_move)
        paint_ai_objs(&painter, m_storage_move->values());
    if (m_storage)
        paint_ai_objs(&painter, m_storage->values());
    paintPreviewBorder(&painter);
    return result;
}

void VideoWidget::initializeGL()
{
    initializeOpenGLFunctions();
    connect(this,&VideoWidget::imageClicked,this,&VideoWidget::onImageClicked, Qt::UniqueConnection);
    glClearColor(0, 0, 0, 1);
    statTimer.start();
}

void VideoWidget::mousePressEvent(QMouseEvent *event)
{
    setFocus();
    if (event->button() == Qt::RightButton) {
        m_mouseSteering = true;
        m_mouseOrigin = m_mousePosition = event->position();
        m_mouseSectors = {};
        m_wheelRemainder = 0;
        setCursor(Qt::CrossCursor);
        emit mouseSteeringStarted();
        update();
        event->accept();
        return;
    }
    if (m_mouseSteering) { event->accept(); return; }
    if (event->button() == Qt::LeftButton)
        if(enable_click) emit imageClicked(event->pos());
    if (event->button() == Qt::MiddleButton)test_search=!test_search;
    QOpenGLWidget::mousePressEvent(event);
}

void VideoWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_mouseSteering) {
        if (!(event->buttons() & Qt::RightButton)) {
            stopMouseSteering();
        } else {
            m_mousePosition = event->position();
            const QPointF delta = m_mousePosition - m_mouseOrigin;
            // Ten independent steps per axis, each 10 logical pixels wide.
            const auto sector = [](double value) {
                return int(std::clamp(value / 10.0, -10.0, 10.0));
            };
            const QPoint sectors(sector(delta.x()), sector(delta.y()));
            if (sectors != m_mouseSectors) {
                m_mouseSectors = sectors;
                emit mouseSteeringStep(sectors);
            }
        }
        update();
        event->accept();
        return;
    }
    if (enable_click && (event->buttons() & Qt::LeftButton))
        emit imageClicked(event->pos());
    QOpenGLWidget::mouseMoveEvent(event);
}

void VideoWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton && m_mouseSteering) {
        stopMouseSteering();
        event->accept();
        return;
    }
    QOpenGLWidget::mouseReleaseEvent(event);
}

void VideoWidget::wheelEvent(QWheelEvent *event)
{
    if (m_mouseSteering && (event->buttons() & Qt::RightButton)) {
        m_wheelRemainder += event->angleDelta().y();
        const int steps = m_wheelRemainder / 120;
        m_wheelRemainder %= 120;
        if (steps) emit mouseZoomSteps(steps);
    }
    // Do not propagate the wheel to zoom controls outside the RMB gesture.
    event->accept();
}

void VideoWidget::stopMouseSteering()
{
    if (!m_mouseSteering) return;
    m_mouseSteering = false;
    m_mouseSectors = {};
    m_wheelRemainder = 0;
    unsetCursor();
    emit mouseSteeringStopped();
    update();
}

void VideoWidget::focusOutEvent(QFocusEvent *event)
{
    stopMouseSteering();
    QOpenGLWidget::focusOutEvent(event);
}

void VideoWidget::hideEvent(QHideEvent *event)
{
    stopMouseSteering();
    QOpenGLWidget::hideEvent(event);
}

bool VideoWidget::event(QEvent *event)
{
    if (event->type() == QEvent::WindowDeactivate || event->type() == QEvent::UngrabMouse)
        stopMouseSteering();
    return QOpenGLWidget::event(event);
}

void VideoWidget::paintMouseSteering(QPainter* painter)
{
    if (!m_overlaysEnabled) return;
    if (!m_mouseSteering || !m_showMouseSteering) return;
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(QPen(QColor(255, 210, 70, 200), 1));
    painter->setBrush(QColor(255, 210, 70, 18));
    painter->drawRect(QRectF(m_mouseOrigin - QPointF(100, 100), QSizeF(200, 200)));
    painter->drawRect(QRectF(m_mouseOrigin - QPointF(10, 10), QSizeF(20, 20)));
    painter->drawLine(m_mouseOrigin, m_mousePosition);
    for (int i = -10; i <= 10; ++i) {
        painter->drawLine(m_mouseOrigin + QPointF(i * 10, -3), m_mouseOrigin + QPointF(i * 10, 3));
        painter->drawLine(m_mouseOrigin + QPointF(-3, i * 10), m_mouseOrigin + QPointF(3, i * 10));
    }
    painter->setBrush(QColor(255, 210, 70));
    painter->drawEllipse(m_mousePosition, 4, 4);
    painter->restore();
}

void VideoWidget::onImageClicked(QPoint p)
{
    QPointF coef = QPointF(width()  / double(curr_size.width()), height() / double(curr_size.height()));
    click_pos = localToGlobal(ptz_angle,p,zoom,size(),this->objectName().contains("RGB"));
        //emit moveTestToCommand(click_pos);
    qDebug()<<this->objectName()<<click_pos<<ptz_angle;
    emit moveToCommand(click_pos);
}

void VideoWidget::setFrame(quint64 time,const QImage& img)
{
    d_frame_time=time;
    if(clear_image){
        temp_image=img;
        update();
    }
    frameCounter++;
    qint64 elapsed = statTimer.elapsed();
    if(elapsed >= 1000){
       fps = frameCounter * 1000.0 / elapsed;
       pps = metaCounter  * 1000.0 / elapsed;
       dps = paintCounter  * 1000.0 / elapsed;
       frameCounter = 0;
       metaCounter = 0;
       paintCounter = 0;
       statTimer.restart();
    }

    QueuedFrame qf;
    qf.frameId = time;
    // QImage is implicitly shared: the receiver already owns a safe deep copy.
    qf.image = img;
    if (curr_size != img.size() && !img.isNull()) {
        curr_size = img.size();
        emit update_size(curr_size);
    }
//    if(this->objectName()=="openGLWidget_RGB")qDebug()<<"setFrame "<<QString::number(time/100000.0,'d',3)<<m_frameQueue.length();
    QMutexLocker locker(&m_queueMutex);
    m_frameQueue.enqueue(qf);

    if (m_frame_time != 0) {
        const qint64 telemetryLag = qint64(time) - qint64(m_frame_time);
//        qDebug() << objectName() << "Telemetry lag (latest video TS - telemetry TS):"
//                 << QString::number(telemetryLag/90000.0,'d',3) << "timestamp ticks"
//                 << "latest video TS:" << time << "telemetry TS:" << m_frame_time;
    }

    while(m_frameQueue.size() > MAX_QUEUE){
        // If telemetry has fallen behind the entire queue, keep playback moving
        // by displaying the oldest frame just before it leaves the queue.
        const QueuedFrame oldest = m_frameQueue.dequeue();
        if (!clear_image && m_frame_time != 0 && oldest.frameId > m_frame_time)
            m_displayFrame = oldest.image;
    }
    update();

}


QImage VideoWidget::findFrameById(quint64 frameId)
{
    QMutexLocker locker(&m_queueMutex);
    if (m_frameQueue.isEmpty())
        return {};

    // Until metadata arrives, displaying the freshest frame is preferable to
    // accumulating startup latency.
    if (frameId == 0) {
        QImage newest = m_frameQueue.back().image;
        m_frameQueue.clear();
        return newest;
    }

    // Pick the newest video frame that is not newer than the metadata.  Consume
    // all older candidates so playback cannot fall progressively behind.
    QImage candidate;
    while (!m_frameQueue.isEmpty() && m_frameQueue.head().frameId <= frameId)
        candidate = m_frameQueue.dequeue().image;

    return candidate;
}

void VideoWidget::setMeta(const QJsonObject &obj)
{
    QJsonArray ai =obj["ai"].toArray();
    metaCounter++;

    QJsonObject camera;
    if(this->objectName().contains("RGB")){
        camera =obj["rgb"].toObject();
    }else{
        camera =obj["ir"].toObject();
    }
    m_frame_time = camera["ts"].toInteger();
//    qDebug()<<"setMeta "<<QString::number(m_frame_time/90000.0,'d',3)<<obj;
//    qDebug()<<"setMeta  "<<debugFrameQueue().contains(QString::number(m_frame_time,'d',0))
//           <<QString::number(m_frame_time,'d',0)<<"|"<<debugFrameQueue()<<"||"
//          <<(debugFrameQueue().split(' ').last().toLongLong()-m_frame_time)/100000.0;

    zoom=camera["zoom"].toDouble();
    QJsonObject station =obj["st"].toObject();

    QJsonArray st_ang=station["gyro"].toArray();
    if(st_ang.count()>2){
        st_angle.setX(st_ang[0].toDouble());
        st_angle.setY(st_ang[1].toDouble());
        st_angle.setZ(st_ang[2].toDouble());
    }

    QJsonArray ptz_ang=station["ang"].toArray();
    if(ptz_ang.count()>3){
        ptz_angle.setX(ptz_ang[0].toDouble());
        ptz_angle.setY((-ptz_ang[1].toDouble()>-90)?-ptz_ang[1].toDouble():360-ptz_ang[1].toDouble());
        ptz_speed.setX(ptz_ang[2].toDouble());
        ptz_speed.setY(-ptz_ang[3].toDouble());
    }

    QJsonArray pos=station["pos"].toArray();
    if(pos.count()>2){
        st_pos.setX(pos[0].toDouble());
        st_pos.setY(pos[1].toDouble());
        st_pos.setZ(pos[2].toDouble());
    }

    QJsonArray mag=station["mag"].toArray();
    if(mag.count()>2){
        st_mag.setX(mag[0].toDouble());
        st_mag.setY(mag[1].toDouble());
        st_mag.setZ(mag[2].toDouble());
    }


    QJsonObject dist = obj["laser"].toObject();
    st_dist =dist["distance"].toDouble();

    emit set_meta_a_p_g_m(ptz_angle,st_pos,st_angle,st_mag);

    QJsonObject target = obj["target"].toObject();
    QJsonArray angle_coor=target["pos"].toArray();
    if(angle_coor.count()>1) target_obj.angle_center = QPoint(target["a_pos"][0].toDouble(),target["a_pos"][1].toDouble());
    QJsonArray pix_coor=target["pos"].toArray();
    if(pix_coor.count()>1) target_obj.pixel_center = QPoint(target["f_pos"][0].toDouble(),target["f_pos"][1].toDouble());
    target_obj.classname=target["name"].toInt();
    target_obj.id=target["id"].toInt();
//    qDebug()<<"setMetaTarget  "<<target<<target_obj.classname<<target_obj.id<<target_obj.pixel_center;


    QMutexLocker locker(&m_queueMutex);
    work_with_storage(ai);
    {
        QMutexLocker snapshotLock(&m_recordingTelemetryMutex);
        m_recordingTelemetry=obj;
        m_recordingCamera=objectName().contains("RGB") ? "rgb" : "ir";
    }
    emit update_list();
}

void VideoWidget::work_with_storage(QJsonArray ai){
    QPointF coef = QPointF(width()  / double(curr_size.width()), height() / double(curr_size.height()));
    for (const QJsonValue& var : ai){
        Detection ai_obj;
        ai_obj.classname=var["class"].toInt();
        ai_obj.id=var["id"].toInt();
        ai_obj.prec=var["conf"].toDouble();
        QJsonArray rect =  var["box"].toArray();
        ai_obj.box= QRect(rect[0].toInt(), rect[1].toInt(), rect[2].toInt(), rect[3].toInt());
//        if(ai_obj.classname!=-1){
            ai_obj.display_coef=coef;
            ai_obj.angle_center=localToGlobal(ptz_angle,ai_obj.get_local_center(),zoom,size(), this->objectName().contains("RGB"));
//        }
        update_storage(ai_obj);
    }
    QStringList olds;
    foreach(QString key, m_storage_move->keys()){
        Detection &val = (*m_storage_move)[key];
        if(val.old>30)olds.append(key);
        else val.old++;
    }
    foreach (QString key, olds)m_storage_move->remove(key);

    olds.clear();
    foreach (QString key, m_storage->keys()) {
        Detection &val = (*m_storage)[key];
        if(val.old>30)olds.append(key);
        else val.old++;
    }
    foreach (QString key, olds)m_storage->remove(key);
}

void VideoWidget::update_storage(Detection obj){
    QMap<QString, Detection>* temp_storage =m_storage_move;
    QString name="move "+QString::number(obj.id);
    if(obj.classname!=-1){
        name=m_objectNames.value(obj.classname)+" "+QString::number(obj.id);
        temp_storage =m_storage;
    }
    if(!(*temp_storage).contains(name)){
        (*temp_storage).insert(name,obj);
    }else{
        (*temp_storage)[name].old=0;
        (*temp_storage)[name].box=obj.box;
        (*temp_storage)[name].angle_center=obj.angle_center;
    }
}
void VideoWidget::update_focus(Detection focus)
{
    main_obj=focus;
}

void VideoWidget::paintPreviewBorder(QPainter *painter)
{
    if (!m_previewBorder) return;
    painter->save();
    painter->setPen(QPen(QColor("#58A6FF"), 2));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(QRectF(rect()).adjusted(1, 1, -1, -1));
    painter->restore();
}

void VideoWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);
    QPainter painter(this);
    if(zoom==-1) { paintMouseSteering(&painter); paintPreviewBorder(&painter); return; }
    if(clear_image){
        painter.drawImage(rect(),temp_image);
    }else{
        QImage candidate = findFrameById(m_frame_time);
        if (!candidate.isNull())
            m_displayFrame = candidate;
        if (!m_displayFrame.isNull())
            painter.drawImage(rect(),m_displayFrame);
    }

    paintCounter++;
    if(this->objectName().contains("RGB")){
        emit set_meta_f_z(m_frame_time,zoom);
        emit set_meta_a_p_g_m(ptz_angle,st_pos,st_angle,st_mag);
        emit set_meta_d(st_dist);
    }
//    paintCounter++;
    paint_overlay(&painter);
    paint_ai_objs(&painter,m_storage_move->values());
    paint_ai_objs(&painter,m_storage->values());
    paintMouseSteering(&painter);
    paintPreviewBorder(&painter);
    if (get_clear_frame){
        getTimeNs(2);
        get_clear_frame=false;
    }
//    qDebug() << "Frame get processing 1:" <<  QString::number( getTimeNs(1)/1000000000.0,'d',3)<<" s";
}

void VideoWidget::paint_overlay(QPainter* painter)
{
    if (!m_overlaysEnabled) return;
    if(show_aim){
        draw_aim(painter,QPoint(rect().width() /2,rect().height()/2));
        draw_test_marker(painter, ptz_angle, QPoint(0,0), zoom, size(), this->objectName().contains("RGB"));
        draw_test_marker(painter, ptz_angle, click_pos, zoom, size(), this->objectName().contains("RGB"));
        drawMotionVector(painter,ptz_speed);
    }

    if(show_text)draw_text(painter);
    if(show_degree){
        draw_azimuth_scale(painter,ptz_angle.x(),zoom);
        drawPitchScale(painter,ptz_angle.y(),zoom);
    }
    if (m_precisionMode && !m_precisionIcon.isNull())
        painter->drawImage(QRect(width()/2-24, height()/2+84, 48, 48), m_precisionIcon);
}

void VideoWidget::draw_text(QPainter * painter)
{
    QFont font("Consolas", 12);
    font.setStyleHint(QFont::Monospace);
    font.setWeight(QFont::Bold);
    painter->setFont(font);
    painter->setPen(QPen(green_overlay, 2));
    painter->drawText(10, 30,  QString("ZOOM :%1 x").arg(zoom, 3));
    painter->drawText(10, 50,  QString("FOCUS:%1 mm").arg(zoom*6, 3));
    painter->drawText(10, 120, QString("FPS: %1").arg(fps, 3,'d',1));
    painter->drawText(10, 140, QString("PPS: %1").arg(pps, 3,'d',1));
    painter->drawText(10, 160, QString("DPS: %1").arg(dps, 3,'d',1));

    painter->drawText(10, 250, QString("TS : %1").arg(m_frame_time/100000.0, 9,'d',3));

    painter->drawText(QRect(0, height()/2+45, width(), painter->fontMetrics().height()+4),
                      Qt::AlignCenter, QString("%1 m").arg(st_dist, 0,'f',1));

}

void VideoWidget::draw_aim(QPainter * painter,QPoint aim,QPointF coef, QColor color)
{
    painter->setPen(QPen(color, 2));
    aim= QPointF(aim.x()*coef.x(),aim.y()*coef.y()).toPoint();
    int cross_size=30;
    int r2 = 15;
    int r3 = 5;

    painter->drawLine( aim.x()-cross_size, aim.y(),  aim.x()-r2, aim.y());
    painter->drawLine( aim.x()+r2, aim.y(), aim.x()+cross_size, aim.y());

    painter->drawLine( aim.x(), aim.y()-cross_size, aim.x(),aim.y()-r2);
    painter->drawLine( aim.x(), aim.y()+cross_size, aim.x(),aim.y()+r2);

    painter->drawLine(QPoint(aim.x()-r3,aim.y()),QPoint(aim.x()+r3,aim.y()));
    painter->drawLine(QPoint(aim.x(),aim.y()-r3),QPoint(aim.x(),aim.y()+r3));
}

void VideoWidget::paint_ai_objs(QPainter * painter, QVector<Detection> objects)
{
    if (!m_overlaysEnabled) return;
    if(objects.length()==0){ objects=last_detection;
    }else{ last_detection=objects;}
    for (const Detection& det : objects)
    {
        if(det.classname==-1&& !motion_visible)continue;
        if(det.classname==0 && !motion_planes)continue;
        if(det.classname==1 && !motion_birds)continue;
        if(det.classname==2 && !motion_drones)continue;
        if(det.classname==3 && !motion_cars)continue;
        if(det.classname==4 && !motion_mans)continue;
        if(det.old>2)continue;

        QRect r(int(det.box.x()),int(det.box.y()),int(det.box.width()),int(det.box.height()) );
        QColor color;
        if(det.classname==0)color=Qt::red;
        else if(det.classname==1)color=Qt::black;
        else if(det.classname==2)color=Qt::green;
        else if(det.classname==3)color=Qt::white;
        else if(det.classname==4)color=Qt::blue;
        else if(det.classname==-1)color=Qt::yellow;
        else color=Qt::gray;
        double pen_size=1;
        if(target_obj.id!=-1 && det.id==target_obj.id){
            pen_size=4;
            draw_aim(painter,target_obj.pixel_center.toPoint(),det.display_coef,red_overlay);
            draw_test_marker(painter, ptz_angle, target_obj.angle_center, zoom, size(), this->objectName().contains("RGB"));
//            draw_aim(painter,target_obj.angle_center.toPoint(),det.display_coef,red_overlay);

//            drawVector(painter,det.get_center(),target_obj.pixel_center.toPoint(),det.display_coef);
            qDebug()<<"target"<<target_obj.angle_center<<target_obj.pixel_center.toPoint();
//            drawVector(painter,det.get_center(),target_obj.pixel_center.toPoint());

        }
        painter->setPen(QPen(color,pen_size));

        painter->drawRect(QRect (r.topLeft().x()*det.display_coef.x(), r.topLeft().y()*det.display_coef.y(),
                                 r.width()*det.display_coef.x(), r.height()*det.display_coef.y()));

        QString label = QString("%1 %2 (p:%3)").arg(det.classname==-1?"move":m_objectNames.value(det.classname)).arg(det.id).arg(det.prec, 0, 'f', 2);
        QFontMetrics fm = painter->fontMetrics();
        int textHeight = fm.height();
        painter->drawText(QPointF(r.bottomLeft().x()*det.display_coef.x(),r.bottomLeft().y()*det.display_coef.y()) + QPoint(0, textHeight), label);
    }
}
