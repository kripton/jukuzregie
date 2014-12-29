#include "videoappsrc.h"

VideoAppSrc::VideoAppSrc(QObject *parent) :
    QObject(parent)
{
    enableBlock(false);
    setStreamType(QGst::AppStreamTypeStream);
    setLive(true);
}

void VideoAppSrc::needData(uint length)
{
    //qDebug() << "VIDEOAPPSOURCE NEED DATA. Length:" << length;

    emit sigNeedData(length);
}

void VideoAppSrc::enoughData()
{
    qDebug() << "VIDEOAPPSOURCE ENOUGH DATA";
}

void VideoAppSrc::pushVideoBuffer(QByteArray data)
{
    QGst::BufferPtr buf = QGst::Buffer::create(data.size());
    QGst::MapInfo map;
    buf->map(map, QGst::MapWrite);
    memcpy(map.data(), data.data(), map.size());
    buf->unmap(map);

    pushBuffer(buf);
}
