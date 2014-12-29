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
    Q_UNUSED(length);

    //qDebug() << "VIDEOAPPSOURCE NEED DATA. Length:" << length;

    if (!buffer.isNull())
    {
        buffer.clear();
    }
    buffer = QGst::Buffer::create(640*360*4);
    buffer->map(mapInfo, QGst::MapWrite);

    emit sigNeedData(buffer->size(), (char*)mapInfo.data());
}

void VideoAppSrc::enoughData()
{
    qDebug() << "VIDEOAPPSOURCE ENOUGH DATA";
}

void VideoAppSrc::pushVideoBuffer()
{
    if (buffer.isNull())
    {
        return;
    }

    buffer->unmap(mapInfo);
    pushBuffer(buffer);
}
