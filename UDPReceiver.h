#pragma once

#include <QObject>
#include <QUdpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QHostAddress>

class CommandReceiver : public QObject
{
    Q_OBJECT

public:
    explicit CommandReceiver(QObject* parent = nullptr)
        : QObject(parent)
    {
        socket = new QUdpSocket(this);

        connect(socket, &QUdpSocket::readyRead,
                this, &CommandReceiver::onReadyRead);
    }

    bool bind(QString addr,int port)
    {
        // слушаем все интерфейсы
        bool ok = socket->bind(QHostAddress::Any, port);
//        bool ok = socket->bind(QHostAddress(addr), port);
        // qDebug() << "UDP:" <<addr<< port;
        if (!ok) {
            qDebug() << "UDP bind failed:" << socket->errorString();
        } else {
            // qDebug() << "UDP listening on port:" <<addr<< port;
        }

        return ok;
    }

signals:
    void jsonReceived(const QJsonObject& obj);
    void metadataReceived(quint32 frame, double zoom);
    void metaObjectsReceived(const QJsonObject& obj);

private slots:
    void onReadyRead()
    {
        while (socket->hasPendingDatagrams())
        {
            QByteArray datagram;
            datagram.resize(socket->pendingDatagramSize());

            QHostAddress sender;
            quint16 senderPort;

            socket->readDatagram(datagram.data(), datagram.size(),
                                 &sender, &senderPort);

            // 🔥 DEBUG
//            qDebug() << "UDP RAW:" << datagram;
            int start = datagram.indexOf('{');
            if (start >= 0) datagram = datagram.mid(start);
            // ---- JSON parse ----
            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(datagram, &err);

            if (err.error == QJsonParseError::NoError && doc.isObject())
            {
                QJsonObject obj = doc.object();
                emit metaObjectsReceived(obj);
            }
            else
            {
                qDebug() << "UDP JSON error:"
                         << err.errorString()
                         << datagram;
            }
        }
    }

private:
    QUdpSocket* socket;
};
