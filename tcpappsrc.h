#ifndef TCPAPPSRC_H
#define TCPAPPSRC_H

#include <QObject>
#include <QTcpSocket>
#include <QFile>

#include <QGlib/Error>
#include <QGlib/Connect>
#include <QGst/Init>
#include <QGst/Bus>
#include <QGst/Pipeline>
#include <QGst/Parse>
#include <QGst/Message>
#include <QGst/Memory>
#include <QGst/Buffer>
#include <QGst/Utils/ApplicationSource>

class TcpAppSrc : public QObject, public QGst::Utils::ApplicationSource
{
    Q_OBJECT
public:
    explicit TcpAppSrc(QObject *parent = 0);
    bool start(QString host, quint16 port, QString dumpFileName = QString(""));
    void stop();

    virtual void needData (uint length);
    virtual void enoughData();

private:
    QTcpSocket sock;
    QFile file;

private slots:
    void readyRead();

signals:
    void sigNeedData(uint length, char* data);
};

#endif // TCPAPPSRC_H
