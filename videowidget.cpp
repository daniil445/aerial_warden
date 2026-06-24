#include "videowidget.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QPainter>
#include <QSettings>

VideoWidget::VideoWidget(QWidget *parent)
    : QOpenGLWidget(parent)
{
}

void VideoWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0, 0, 0, 1);
    QImage img(1280, 720, QImage::Format_RGB888);
    img.fill(Qt::black);
    setFrame(0,img);
    m_image= img;
}

void VideoWidget::setFrame(quint64 time,const QImage& img)
{
    QueuedFrame qf;
    qf.frameId = time;   // твоя метка
    qf.image = img.copy();
    if(curr_size!=img.size()) emit update_size(this->size());// img.size());
    QMutexLocker locker(&m_queueMutex);

    m_frameQueue.enqueue(qf);

//    qDebug()<<"setFrame"<<(quint16)time<<m_frameQueue.size();
    if(m_frameQueue.size() > MAX_QUEUE){
        QMutexLocker locker(&m_mutex);
        QueuedFrame temp =m_frameQueue.dequeue();
        m_image = temp.image;
        d_frame_time = temp.frameId;
        update();
    }
}

void VideoWidget::setMeta(const QJsonObject &obj)
{
//    qDebug()<<"meta"<<obj;
    QJsonObject camera =obj["rgb"].toObject();
    quint32 meta_seq = camera["ts"].toInteger();
    c_zoom=camera["zoom"].toDouble();

    QJsonObject station =obj["st"].toObject();

    QJsonArray st_ang=station["gyro"].toArray();
    if(st_ang.count()>2){
        st_angle.setX(st_ang[0].toDouble());
        st_angle.setY(st_ang[1].toDouble());
        st_angle.setZ(st_ang[2].toDouble());
    }

    QJsonArray ptz_ang=station["ang"].toArray();
    if(ptz_ang.count()>1){
        ptz_angle.setX(ptz_ang[0].toDouble());
        ptz_angle.setY(ptz_ang[1].toDouble());
    }

    QJsonArray pos=station["pos"].toArray();
    if(pos.count()>2){
        st_pos.setX(pos[0].toDouble());
        st_pos.setY(pos[1].toDouble());
        st_pos.setZ(pos[2].toDouble());
    }

    QJsonObject dist = obj["laser"].toObject();
    st_dist =dist["distance"].toDouble();

    Detection ai_obj;
    Global_coor ai_pos;
    QJsonArray ai =obj["ai"].toArray();
    double sx = width()  / double(m_image.width());
    double sy = height() / double(m_image.height());
    QVector<Detection> ai_objs;
    for (const QJsonValue& var : ai)
    {
        ai_obj.classname=var["class"].toInt();
        ai_obj.id=var["id"].toInt();
        ai_obj.prec=var["conf"].toDouble();
        QJsonArray rect =  var["box"].toArray();
        QRect r(rect[0].toInt()*sx, rect[1].toInt()*sy, rect[2].toInt()*sx, rect[3].toInt()*sy);
        ai_obj.box=r;

        ai_pos.graphic_pos=ai_obj.get_center();
        if(ptz_ang.count()>1)ai_pos.rotator_angle=QVector2D(ptz_ang[0].toDouble(),ptz_ang[1].toDouble());
        if(pos.count()>2)ai_pos.station_pos=QVector3D(pos[0].toDouble(),pos[1].toDouble(),pos[2].toDouble());
        if(st_ang.count()>2)ai_pos.station_angle=QVector3D(st_ang[0].toDouble(),st_ang[1].toDouble(),st_ang[2].toDouble());
        ai_pos.distance=st_dist;

        ai_obj.history.append(ai_pos);
        ai_objs.append(ai_obj);
    }

    QueuedMeta meta;
    meta.metaId = meta_seq;
    meta.ai_objs = ai_objs;
    QMutexLocker locker(&m_queueMutex);
    m_metaQueue.enqueue(meta);
    while (m_metaQueue.size() > MAX_QUEUE) m_metaQueue.dequeue();
}

void VideoWidget::show_movement(bool motion_visible)
{
    this->motion_visible = motion_visible;
}

void VideoWidget::show_planes(bool state)
{
    motion_planes= state;
}

void VideoWidget::show_drones(bool state)
{
    motion_drones= state;
}

void VideoWidget::show_birds(bool state)
{
    motion_birds = state;
}

void VideoWidget::show_cars(bool state)
{
    motion_cars= state;
}

void VideoWidget::show_mans(bool state)
{
    motion_mans= state;
}

 QVector<Detection> VideoWidget::findMetaByFrameId(int frameId)
{
    while (!m_metaQueue.isEmpty())
    {
        const QueuedMeta& m = m_metaQueue.head();

        if (m.metaId < frameId)
        {
            m_metaQueue.dequeue();
            continue;
        }
        if (m.metaId == frameId)
        {
            return m_metaQueue.dequeue().ai_objs;
        }
        break;
    }
    return last_detection;
 }


void VideoWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);
    QPainter painter(this);
    QMutexLocker lock(&m_mutex);
    QImage imgToDraw = m_image;
    painter.drawImage(rect(),imgToDraw);
    paint_overlay(&painter);

    emit set_meta_f_z(d_frame_time,c_zoom);
    emit set_meta_a_p(ptz_angle,st_pos);
    emit set_meta_d(st_dist);

    paint_ai_objs(&painter);
}


void VideoWidget::paint_overlay(QPainter* painter)
{
    painter->setPen(QPen(Qt::green, 2));
    double x= rect().width() /2;
    double y= rect().height()/2;
    double cross_size=50;
    painter->drawLine(QPoint(x-cross_size,y),QPoint(x+cross_size,y));
    painter->drawLine(QPoint(x,y-cross_size),QPoint(x,y+cross_size));
//    painter->drawEllipse(x,y,20,20);
    draw_azimuth_scale(painter,ptz_angle.x());
    drawPitchScale(painter,ptz_angle.y());
}

void VideoWidget::paint_ai_objs(QPainter * painter)
{
    QVector<Detection> objects=findMetaByFrameId(d_frame_time);
    if(objects.length()==0){
        objects=last_detection;
    }else{
        last_detection=objects;
    }
    emit send_obj_list(objects);
    for (const Detection& det : objects)
    {
        if(det.classname==-1&& !motion_visible)continue;
        if(det.classname==0 && !motion_planes)continue;
        if(det.classname==1 && !motion_birds)continue;
        if(det.classname==2 && !motion_drones)continue;
        if(det.classname==3 && !motion_cars)continue;
        if(det.classname==4 && !motion_mans)continue;

        QRect r(int(det.box.x()),int(det.box.y()),int(det.box.width()),int(det.box.height()) );

        if(det.classname==0)painter->setPen(Qt::red);
        else if(det.classname==1)painter->setPen(Qt::black);
        else if(det.classname==2)painter->setPen("orange");
        else if(det.classname==3)painter->setPen(Qt::white);
        else if(det.classname==4)painter->setPen(Qt::blue);
        else if(det.classname==-1)painter->setPen(Qt::yellow);
        else painter->setPen(Qt::gray);
        painter->drawRect(r);

        QSettings settings("config.ini", QSettings::IniFormat);
        QStringList temp =settings.value("obj_name").toStringList();
        QString label = QString("%1 %2 (p:%3)").arg(det.classname==-1?"move":temp.value(det.classname)).arg(det.id).arg(det.prec, 0, 'f', 2);
        QFontMetrics fm = painter->fontMetrics();
        int textHeight = fm.height();
        painter->drawText(r.bottomLeft() + QPoint(0, textHeight), label);
    }
}

void VideoWidget::draw_azimuth_scale( QPainter* painter, double headingDeg)
{
    const int y = 30;
    const int scaleWidth = width() * 0.8;
    const int centerX = width() / 2;
    const int leftX = centerX - scaleWidth / 2;
    painter->setPen(QPen(Qt::green, 2));
    painter->drawLine(leftX, y, leftX + scaleWidth, y);
    const double visibleDeg = 120.0;
    const double pxPerDeg = scaleWidth / visibleDeg;
    QFontMetrics fm(painter->font());
    for (int deg = -180; deg <= 540; deg += 5)
    {
        double delta = deg - headingDeg;
        while (delta > 180) delta -= 360;
        while (delta < -180) delta += 360;
        if (std::abs(delta) > visibleDeg / 2.0) continue;
        int x = centerX + delta * pxPerDeg;
        bool major = (deg % 30 == 0);
        int tickHeight = major ? 15 : 8;
        painter->drawLine(x, y, x, y + tickHeight);
        if (major){
            int norm = deg % 360;
            if (norm < 0) norm += 360;
            QString text;
            switch (norm){
            case 0:   text = "N";  break;
            case 45:  text = "NE"; break;
            case 90:  text = "E";  break;
            case 135: text = "SE"; break;
            case 180: text = "S";  break;
            case 225: text = "SW"; break;
            case 270: text = "W";  break;
            case 315: text = "NW"; break;
            default:
                text = QString::number(norm);
            }
            int tw = fm.horizontalAdvance(text);
            painter->drawText( x - tw / 2, y + tickHeight + fm.height(), text);
        }
    }
    painter->setPen(QPen(Qt::red, 3));
    painter->drawLine(centerX, y - 10, centerX, y + 15);
    painter->drawText(centerX - 8, y - 15, QString("%1°").arg(headingDeg, 0, 'f', 1));
}

void VideoWidget::drawPitchScale(QPainter* painter, double pitchDeg)
{
    const int x = width() - 50;
    const int scaleHeight = height() * 0.8;
    const int centerY = height() / 2;
    const int topY = centerY - scaleHeight / 2;
    painter->setPen(QPen(Qt::green, 2));
    painter->drawLine(x, topY, x, topY + scaleHeight);
    const double visibleDeg = 60.0;
    const double pxPerDeg = scaleHeight / visibleDeg;
    QFontMetrics fm(painter->font());
    while (pitchDeg > 180) pitchDeg -= 360;
    while (pitchDeg < -180) pitchDeg += 360;
    for (int deg = -90; deg <= 90; deg += 5){
        double delta = -deg + pitchDeg;
        if (std::abs(delta) > visibleDeg / 2.0) continue;
        int y = centerY - delta * pxPerDeg;
        bool major = (deg % 10 == 0);
        int tickLen = major ? 15 : 8;
        painter->drawLine(x - tickLen,y,x,y);
        if (major){
            QString text = QString::number(deg);
            painter->drawText( x - tickLen - fm.horizontalAdvance(text) - 5, y + fm.height() / 3, text);
        }
    }
    painter->setPen(QPen(Qt::red, 3));
    painter->drawLine( x - 15, centerY, x + 10, centerY);
    painter->drawText( x - 60, centerY +5, QString("%1°").arg(pitchDeg, 0, 'f', 1)
    );
}
