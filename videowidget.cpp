#include "videowidget.h"
#include <QJsonArray>
#include <QJsonObject>

#include <QMouseEvent>
#include <QSettings>

VideoWidget::VideoWidget(QWidget *parent)
    : QOpenGLWidget(parent)
{
}

void VideoWidget::initializeGL()
{
    initializeOpenGLFunctions();
    connect(this,&VideoWidget::imageClicked,this,&VideoWidget::onImageClicked);
    glClearColor(0, 0, 0, 1);
//    QImage img(1920, 1080, QImage::Format_RGB888);
//    img.fill(Qt::black);
//    setFrame(0,img);
//    m_image= img;
    statTimer.start();
}

void VideoWidget::mousePressEvent(QMouseEvent *event)
{
    setFocus();
    if (event->button() == Qt::LeftButton)
        emit imageClicked(event->pos());

    QOpenGLWidget::mousePressEvent(event);
}

void VideoWidget::onImageClicked(QPoint p)
{
    click_pos = localToGlobal(ptz_angle,p,getFOV(zoom),size());
    emit moveToCommand(click_pos);
}

void VideoWidget::setFrame(quint64 time,const QImage& img)
{
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
    qf.image = img.copy();
    curr_size=img.size();
//    qDebug()<<"setFrame "<<time;
    QMutexLocker locker(&m_queueMutex);
    m_frameQueue.enqueue(qf);

    if(m_frameQueue.size() > MAX_QUEUE){
        QMutexLocker locker(&m_mutex);
        m_frameQueue.dequeue();
    }
//    qDebug()<<"setFrame "<<time<<img.size()<<m_frameQueue.size();
    update();

}


QImage VideoWidget::findFrameById(qint64 frameId)
{
    QMutexLocker locker(&m_queueMutex);
    QImage lastFrame;
    while (!m_frameQueue.isEmpty())
    {
        const QueuedFrame &frame = m_frameQueue.head();
        lastFrame = frame.image;
        if(frame.frameId < frameId){
            m_frameQueue.dequeue();
            continue;
        }

        if(frame.frameId == frameId) {
            m_lastImage = frame.image;
            m_frameQueue.dequeue();
            return m_lastImage;
        }
        break;
    }

    if(!lastFrame.isNull()){
        m_lastImage = lastFrame;
        return m_lastImage;
    }
    return m_lastImage;
}


void VideoWidget::setMeta(const QJsonObject &obj)
{
    metaCounter++;
    //qDebug()<<"meta"<<obj;
    QJsonObject camera =obj["rgb"].toObject();
    m_frame_time = camera["ts"].toInteger();
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

    QJsonObject dist = obj["laser"].toObject();
    st_dist =dist["distance"].toDouble();

    QMutexLocker locker(&m_queueMutex);
    QJsonArray ai =obj["ai"].toArray();
    work_with_storage(ai);
    emit update_list();
}

void VideoWidget::work_with_storage(QJsonArray ai){
    QPointF coef = QPointF(width()  / double(m_lastImage.width()), height() / double(m_lastImage.height()));
    for (const QJsonValue& var : ai){
        Detection ai_obj;
        ai_obj.classname=var["class"].toInt();
        ai_obj.id=var["id"].toInt();
        ai_obj.prec=var["conf"].toDouble();
        QJsonArray rect =  var["box"].toArray();
        ai_obj.box= QRect(rect[0].toInt(), rect[1].toInt(), rect[2].toInt(), rect[3].toInt());
//        if(ai_obj.classname!=-1){
            ai_obj.display_coef=coef;
            ai_obj.angle_center=localToGlobal(ptz_angle,ai_obj.get_local_center(),getFOV(zoom),size());
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
    QSettings settings("config.ini", QSettings::IniFormat);
    QStringList temp =settings.value("obj_name").toStringList();
    QString name="move "+QString::number(obj.id);
    if(obj.classname!=-1){
        name=temp[obj.classname]+" "+QString::number(obj.id);
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

void VideoWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);
    QPainter painter(this);
//    qDebug()<<"ready for read "<<m_frame_time;
    QMutexLocker lock(&m_mutex);

    emit set_meta_f_z(m_frame_time,zoom);
    emit set_meta_a_p(ptz_angle,st_pos);
    emit set_meta_d(st_dist);
    QImage imgToDraw = findFrameById(m_frame_time);
    painter.drawImage(rect(),imgToDraw);
    paintCounter++;
    paint_overlay(&painter);
    paint_ai_objs(&painter,m_storage_move->values());
    paint_ai_objs(&painter,m_storage->values());
}

void VideoWidget::paint_overlay(QPainter* painter)
{
    if(show_aim){
        draw_aim(painter,QPoint(rect().width() /2,rect().height()/2));
        draw_test_marker(painter, ptz_angle, QPoint(0,0), getFOV(zoom), size());
        draw_test_marker(painter, ptz_angle, click_pos, getFOV(zoom), size());
//        draw_degree(painter, ptz_angle, QPoint(11,10), QPoint(10,11), getFOV(zoom), size());
        drawMotionVector(painter,ptz_speed);
    }

    if(show_text)draw_text(painter);
    if(show_degree){
        draw_azimuth_scale(painter,ptz_angle.x(),zoom);
        drawPitchScale(painter,ptz_angle.y(),zoom);
    }
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

}

void VideoWidget::draw_aim(QPainter * painter,QPoint aim, QColor color)
{
    painter->setPen(QPen(color, 2));
    double cross_size=50;
    painter->drawLine(QPoint(aim.x()-cross_size,aim.y()),QPoint(aim.x()+cross_size,aim.y()));
    painter->drawLine(QPoint(aim.x(),aim.y()-cross_size),QPoint(aim.x(),aim.y()+cross_size));
}

void VideoWidget::paint_ai_objs(QPainter * painter, QVector<Detection> objects)
{
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
        if(det.classname==main_obj.classname && det.id==main_obj.id){
            pen_size=4;
            Detection& temp = (*m_storage)[main_obj.get_name()];
            draw_aim(painter,temp.get_local_center());
            if(temp.kalman_init)kalman_init(temp);
            QPointF filtered = kalman_update(temp,  temp.angle_center, 1.0/30.0);

            draw_aim(painter,main_obj.angle_center.toPoint(),red_overlay);
//            qDebug()<<temp.get_name()<<"filtered"<<filtered<<temp.angle_center;
        }
        painter->setPen(QPen(color,pen_size));

        painter->drawRect(QRect (r.topLeft().x()*det.display_coef.x(), r.topLeft().y()*det.display_coef.y(),
                                 r.width()*det.display_coef.x(), r.height()*det.display_coef.y()));

        QSettings settings("config.ini", QSettings::IniFormat);
        QStringList temp =settings.value("obj_name").toStringList();
        QString label = QString("%1 %2 (p:%3)").arg(det.classname==-1?"move":temp.value(det.classname)).arg(det.id).arg(det.prec, 0, 'f', 2);
        QFontMetrics fm = painter->fontMetrics();
        int textHeight = fm.height();
        painter->drawText(QPointF(r.bottomLeft().x()*det.display_coef.x(),r.bottomLeft().y()*det.display_coef.y()) + QPoint(0, textHeight), label);
    }
}
