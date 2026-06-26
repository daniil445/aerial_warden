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
    curr_size=img.size();
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

    QMutexLocker locker(&m_queueMutex);
    QJsonArray ai =obj["ai"].toArray();
    work_with_storage(ai);
    emit update_list();
}

void VideoWidget::work_with_storage(QJsonArray ai){
    for (const QJsonValue& var : ai){
        Detection ai_obj;
        ai_obj.classname=var["class"].toInt();
        ai_obj.id=var["id"].toInt();
        ai_obj.prec=var["conf"].toDouble();
        QJsonArray rect =  var["box"].toArray();
        ai_obj.box= QRect(rect[0].toInt(), rect[1].toInt(), rect[2].toInt(), rect[3].toInt());
        ai_obj.set_angle_center(ptz_angle,ai_obj.get_center(),QPoint(62,36),curr_size);
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
        Detection &val = (*m_storage_move)[key];
        if(val.old>30)olds.append(key);
        else val.old++;
    }
    foreach (QString key, olds)m_storage_move->remove(key);
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
        (*temp_storage)[name].history.append(obj.angle_center);
        if((*temp_storage)[name].history.length()>10)(*temp_storage)[name].history.pop_back();
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
    QMutexLocker lock(&m_mutex);

    emit set_meta_f_z(d_frame_time,c_zoom);
    emit set_meta_a_p(ptz_angle,st_pos);
    emit set_meta_d(st_dist);

    QImage imgToDraw = m_image;
    painter.drawImage(rect(),imgToDraw);
    paint_overlay(&painter);
    paint_ai_objs(&painter,m_storage_move->values());
    paint_ai_objs(&painter,m_storage->values());
}

void VideoWidget::paint_overlay(QPainter* painter)
{
    if(show_aim)draw_aim(painter,QPoint(rect().width() /2,rect().height()/2));
    if(show_text)draw_text(painter);
    if(show_degree){
        draw_azimuth_scale(painter,ptz_angle.x());
        drawPitchScale(painter,ptz_angle.y());
    }
}

void VideoWidget::draw_text(QPainter * painter)
{
    QFont font("Consolas", 10);
    font.setStyleHint(QFont::Monospace);
    painter->setFont(font);
    painter->setPen(QPen(green_overlay, 2));
    painter->drawText(10, 30, QString("ZOOM :%1 x").arg(c_zoom  , 4, 'f', 1));
    painter->drawText(10, 50, QString("FOCUS:%1 mm").arg(c_zoom*6, 4, 'f', 1));
}

void VideoWidget::draw_aim(QPainter * painter,QPoint aim)
{
    painter->setPen(QPen(green_overlay, 2));
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
        double sx = width()  / double(m_image.width());
        double sy = height() / double(m_image.height());
        if(det.classname==main_obj.classname && det.id==main_obj.id){
            pen_size=4;
            draw_aim(painter,main_obj.get_local_center(QPointF(sx,sy)));
        }
        painter->setPen(QPen(color,pen_size));

        painter->drawRect(QRect (r.topLeft().x()*sx, r.topLeft().y()*sy, r.width()*sx, r.height()*sy));

        QSettings settings("config.ini", QSettings::IniFormat);
        QStringList temp =settings.value("obj_name").toStringList();
        QString label = QString("%1 %2 (p:%3)").arg(det.classname==-1?"move":temp.value(det.classname)).arg(det.id).arg(det.prec, 0, 'f', 2);
        QFontMetrics fm = painter->fontMetrics();
        int textHeight = fm.height();
        painter->drawText(QPointF(r.bottomLeft().x()*sx,r.bottomLeft().y()*sy) + QPoint(0, textHeight), label);
    }
}

void VideoWidget::draw_azimuth_scale( QPainter* painter, double headingDeg)
{
    const int y = 30;
    const int scaleWidth = width() * 0.8;
    const int centerX = width() / 2;
    const int leftX = centerX - scaleWidth / 2;
    painter->setPen(QPen(green_overlay, 2));
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
    painter->setPen(QPen(red_overlay, 3));
    painter->drawLine(centerX, y - 10, centerX, y + 15);
    painter->setPen(QPen(gray_overlay, 3));
    double absAngle = std::abs(headingDeg);
    int deg = static_cast<int>(absAngle);
    double min_full = (absAngle - deg) * 60.0;
    int min = static_cast<int>(min_full);
    double sec = (min_full - min) * 60.0;

    if (headingDeg < 0) deg = -deg;
    painter->drawText(centerX - 46, y - 15, QString("%1° %2' %3''").arg(headingDeg, 3, 'd', 0).arg(min_full, 2, 'd', 0).arg(sec, 2, 'd', 0));
}

void VideoWidget::drawPitchScale(QPainter* painter, double pitchDeg)
{
    const int x = width() - 50;
    const int scaleHeight = height() * 0.8;
    const int centerY = height() / 2;
    const int topY = centerY - scaleHeight / 2;
    painter->setPen(QPen(green_overlay, 2));
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
    painter->setPen(QPen(red_overlay, 3));
    painter->drawLine( x - 15, centerY, x + 10, centerY);
    painter->setPen(QPen(gray_overlay, 3));
    double absAngle = std::abs(pitchDeg);
    int deg = static_cast<int>(absAngle);
    double min_full = (absAngle - deg) * 60.0;
    int min = static_cast<int>(min_full);
    double sec = (min_full - min) * 60.0;

    if (pitchDeg < 0) deg = -deg;
    painter->drawText( x + 10, centerY -15,  QString("%1°").arg(pitchDeg, 3, 'd', 0));
    painter->drawText( x + 10, centerY +5, QString("%1'").arg(min_full, 3, 'd', 0));
    painter->drawText( x + 10, centerY +25, QString("%1''").arg(sec, 3, 'd', 0));
}

