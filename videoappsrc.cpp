#include "videoappsrc.h"

VideoAppSrc::VideoAppSrc(QObject *parent) :
    QObject(parent)
{
    this->width = width;
    this->height = height;
}

void VideoAppSrc::setDimensions(int width, int height)
{
    this->width = width;
    this->height = height;
}

void VideoAppSrc::needData(uint length)
{
    Q_UNUSED(length);

    //qDebug() << "VideoAppSrc NEED DATA. Length:" << length;

    if (!buffer.isNull())
    {
        buffer.clear();
    }
    buffer = QGst::Buffer::create(width*height*4);
    buffer->map(mapInfo, QGst::MapWrite);

    emit sigNeedData(buffer->size(), (char*)mapInfo.data());
}

void VideoAppSrc::enoughData()
{
    //qDebug() << "VideoAppSrc ENOUGH DATA";
}

void VideoAppSrc::pushVideoBuffer()
{
    if (buffer.isNull())
    {
        return;
    }

    buffer->unmap(mapInfo);

    //qDebug() << "VideoAppSrc PUSHBUFFER Length:" << buffer->size();
    pushBuffer(buffer);
}
