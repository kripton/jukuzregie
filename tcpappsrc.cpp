#include "tcpappsrc.h"

TcpAppSrc::TcpAppSrc(QObject *parent) :
    QObject(parent), QGst::Utils::ApplicationSource()
{
    connect(&sock, SIGNAL(readyRead()), this, SLOT(readyRead()));
}

TcpAppSrc::~TcpAppSrc()
{
    stop();
}

bool TcpAppSrc::start(QString host, quint16 port, QString dumpFileName)
{
    if (dumpFileName != "")
    {
        file.setFileName(dumpFileName);
        file.open(QFile::ReadWrite);
    }

    sock.connectToHost(host, port);
    // TODO: Wait until error or connected and return accordingly
    return true;
}

void TcpAppSrc::stop()
{
    qDebug() << "TcpAppSrc STOP";
    sock.disconnectFromHost();
    if (file.isOpen())
    {
        file.close();
    }
}

void TcpAppSrc::needData(uint length)
{
    Q_UNUSED(length)
    //qDebug() << "TCPAPPSOURCE NEED DATA. Length:" << length;
}

void TcpAppSrc::enoughData()
{
    //qDebug() << "TCPAPPSOURCE ENOUGH DATA";
}

void TcpAppSrc::readyRead()
{
    //qDebug() << "TCPAPPSRC: readyRead WITH BYTES:" << sock.bytesAvailable();
    QGst::BufferPtr buf = QGst::Buffer::create(sock.bytesAvailable());
    QGst::MapInfo map;
    buf->map(map, QGst::MapWrite);

    sock.read((char*)map.data(), map.size());

    if (file.isOpen())
    {
        file.write((char*)map.data(), map.size());
    }

    buf->unmap(map);

    pushBuffer(buf);
}
