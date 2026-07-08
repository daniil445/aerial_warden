// CommandSender.h
#include <QUdpSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QDebug>
#include <QVector2D>
#include <QJsonArray>

class CommandSender : public QObject
{
    Q_OBJECT
public:
    QString main_ip;
    int main_port;
    CommandSender(QObject* parent = nullptr)
    {
        socket = new QUdpSocket(this);
    }
    void sendAddress(QString addr,int port)
    {
        main_ip=addr;
        main_port=port;
    }
    void sendZoom(QString cmd,double value)
    {
        QJsonObject obj;
        obj["source"] = "user";
        obj["cmd"] = cmd;
        obj["value"] =value;
        QJsonDocument doc(obj);
        QByteArray data = doc.toJson(QJsonDocument::Compact);
        socket->writeDatagram(data, QHostAddress(main_ip), main_port);
    }
    void sendCmd(QString cmd,QString val=""){
        QJsonObject obj;
        obj["source"] = "user";
        obj["cmd"] = cmd;
        obj["value"] = val;
        QJsonDocument doc(obj);
        QByteArray data = doc.toJson(QJsonDocument::Compact);
        socket->writeDatagram(data, QHostAddress(main_ip), main_port);
    }
    void sendMoveTo(QString value){
        QJsonObject obj;
        obj["source"] = "user";
        obj["cmd"] = "move_to";
        obj["value"] = value;
        QJsonDocument doc(obj);
        QByteArray data = doc.toJson(QJsonDocument::Compact);
        socket->writeDatagram(data, QHostAddress(main_ip), main_port);
    }
    void sendMove(QString value)
    {
        QJsonObject obj;
        obj["source"] = "user";
        obj["cmd"] = "move";
        obj["value"] = value;
        QJsonDocument doc(obj);
        QByteArray data = doc.toJson(QJsonDocument::Compact);
        socket->writeDatagram(data, QHostAddress(main_ip), main_port);
    }
    void sendTarget(QString name,QString dig_name, QPointF pos){
        QJsonArray posArray;
        posArray.append(pos.x());
        posArray.append(pos.y());

        QJsonObject target;
        target["name"] = name;
        target["ids"] = dig_name;
        target["pos"] = posArray;

        QJsonObject obj;
        obj["source"] = "user";
        obj["cmd"] = "fix_target";
        obj["data"] =target;
        QJsonDocument doc(obj);
        QByteArray data = doc.toJson(QJsonDocument::Compact);
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
                    if(entry.ip().toString().contains("192.168.26"))ip=entry.ip().toString();
                    qDebug() << "Interface:" << ip;
                    break;
                }
            }
        }
        if(ip=="")return;
        QJsonObject obj;
        obj["source"] = "user";
        obj["cmd"] = "client_ip";
        obj["value"] = ip;
        QJsonDocument doc(obj);
        QByteArray data = doc.toJson(QJsonDocument::Compact);
        socket->writeDatagram(data, QHostAddress(main_ip), main_port);
    }

private:
    QUdpSocket* socket;
};
