// CommandSender.h
#include <QUdpSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QDebug>

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
    void sendCmd(QString cmd){
        QJsonObject obj;
        obj["source"] = "user";
        obj["cmd"] = cmd;
        obj["value"] = "";
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
    void sendIp()
    {
        QString ip="";
        for (const QNetworkInterface& iface : QNetworkInterface::allInterfaces())
        {
            if (!(iface.flags() & QNetworkInterface::IsUp) ||
                !(iface.flags() & QNetworkInterface::IsRunning) ||
                (iface.flags() & QNetworkInterface::IsLoopBack))
                continue;

            for (const QNetworkAddressEntry& entry : iface.addressEntries())
            {
                if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol)
                {
                    // qDebug() << "Interface:" << iface.humanReadableName()
                    // << "IP:" << entry.ip().toString();
                    if(iface.humanReadableName()=="Ethernet")ip=entry.ip().toString();
                }
            }
        }
        // if(ip=="")return;
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
