#pragma once
// CommandSender.h
#include <QUdpSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QDebug>
#include <QJsonArray>
#include <QPoint>
#include <QPointF>
#include <cmath>

class CommandSender : public QObject
{
    Q_OBJECT
public:
    QString main_ip;
    int main_port = 0;
    CommandSender(QObject* parent = nullptr)
    {
        socket = new QUdpSocket(this);
    }
    void sendAddress(QString addr,int port)
    {
        main_ip=addr;
        main_port=port;
    }
    void sendZoom(const QString& camera, double value)
    {
        if ((camera != "rgb_cam" && camera != "ir_cam") || !std::isfinite(value)
                || value < 1 || value > (camera == "ir_cam" ? 6 : 50)) {
            qWarning() << "Invalid camera zoom:" << camera << value;
            return;
        }
        sendUserUpdate(QJsonObject{{camera, QJsonObject{{"zoom", value}}}});
    }
    void sendZoomSpeed(const QString& camera, double speed)
    {
        if ((camera != "rgb_cam" && camera != "ir_cam") || !std::isfinite(speed)
                || speed < -1 || speed > 1) return;
        sendUserUpdate(QJsonObject{{camera, QJsonObject{{"zoom_speed", speed}}}});
    }
    // Callers can combine supported fields in one UDP datagram.
    void sendUserUpdate(const QJsonObject& update)
    {
        const auto rgb = update.value("rgb_cam").toObject();
        if (rgb.contains("zoom")) {
            const auto value = rgb.value("zoom");
            const double zoom = value.toDouble(-1);
            if (!value.isDouble() || !std::isfinite(zoom) || zoom < 1 || zoom > 50) {
                qWarning() << "RGB zoom must be between 1 and 50:" << value;
                return;
            }
        }
        sendPacket(QJsonObject{{"user", update}});
    }
    void sendCmd(QString cmd,int val=0){
        if (cmd == "stop")
            sendUserUpdate(QJsonObject{{"ptz", QJsonObject{{"stop", true}}}});
        else if (cmd == "init")
            sendUserUpdate(QJsonObject{{"ptz", QJsonObject{{"init", true}}}});
        else if (cmd == "move_stab")
            sendUserUpdate(QJsonObject{{"ptz", QJsonObject{{"stabilization", val != 0}}}});
        else if (cmd == "range_stop")
            sendUserUpdate(QJsonObject{{"rangefinder", QJsonObject{{"mode", 0}}}});
        else if (cmd == "range_find" && val >= 1 && val <= 5)
            sendUserUpdate(QJsonObject{{"rangefinder", QJsonObject{{"mode", val}}}});
        else
            qWarning() << "Unsupported client command:" << cmd << val;
    }
    void sendMoveTo(QString value){
        QJsonArray values;
        if (!parseNumbers(value, 4, values)) return;
        const QJsonObject moveTo{{"position", QJsonArray{values[0], values[1]}},
                                 {"speed", QJsonArray{values[2], values[3]}}};
        sendUserUpdate(QJsonObject{{"ptz", QJsonObject{{"move_to", moveTo}}}});
    }
    void sendMove(QString value)
    {
        QJsonArray values;
        if (!parseNumbers(value, 2, values)) return;
        sendUserUpdate(QJsonObject{{"ptz", QJsonObject{{"move", values}}}});
    }
    void sendTarget(int dig_name,int f_count,int filled,QPointF angle,QPoint pix){
        QJsonObject target;
        target["id"] = dig_name;
        target["f_step"] = f_count;
        target["filled"] = filled != 0;
        QJsonArray pos;
        pos.append(angle.x());
        pos.append(angle.y());
        target["a_pos"] = pos;
        QJsonArray pos_p;
        pos_p.append(pix.x());
        pos_p.append(pix.y());
        target["f_pos"] = pos_p;
        sendUserUpdate(QJsonObject{{"target", target}});
    }

    void sendIp()
    {
        QString ip="";
        for (const QNetworkInterface& iface : QNetworkInterface::allInterfaces())
        {
            if (!(iface.flags() & QNetworkInterface::IsUp) ||
                !(iface.flags() & QNetworkInterface::IsRunning) ||
                (iface.flags() & QNetworkInterface::IsLoopBack))
                continue;
            if (iface.type() != QNetworkInterface::Ethernet)
                continue;
            for (const QNetworkAddressEntry& entry : iface.addressEntries())
            {
                if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol)
                {
                    // << "IP:" << entry.ip().toString();
                    if(entry.ip().toString().contains("192.168.2"))ip=entry.ip().toString();
                    qDebug() << "Interface:" << ip;
                    break;
                }
            }
        }
        if(ip=="")return;
        sendClientIp(ip);
    }

    void sendClientIp(const QString& ip)
    {
        if (!ip.isEmpty() && QHostAddress(ip).protocol() != QAbstractSocket::IPv4Protocol) {
            qWarning() << "Invalid client IPv4 address:" << ip;
            return;
        }
        sendUserUpdate(QJsonObject{{"client_ip", ip}});
    }

private:
    void sendPacket(const QJsonObject& packet)
    {
        const QByteArray data = QJsonDocument(packet).toJson(QJsonDocument::Compact);
        if (socket->writeDatagram(data, QHostAddress(main_ip), main_port) == -1)
            qWarning() << "UDP command send failed:" << socket->errorString();
    }
    static bool parseNumbers(const QString& text, int count, QJsonArray& values)
    {
        const auto parts = text.split('|');
        if (parts.size() != count) {
            qWarning() << "Invalid PTZ value:" << text;
            return false;
        }
        for (const auto& part : parts) {
            bool ok = false;
            const double number = part.toDouble(&ok);
            if (!ok || !std::isfinite(number)) {
                qWarning() << "Invalid PTZ number:" << part;
                return false;
            }
            values.append(number);
        }
        return true;
    }
    QUdpSocket* socket;
};
