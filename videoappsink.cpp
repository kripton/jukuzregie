#include "videoappsink.h"

VideoAppSink::VideoAppSink(QObject *parent) :
    QObject(parent), QGst::Utils::ApplicationSink()
{
    width = 640;
    height = 360;
}

void VideoAppSink::setDimensions(int width, int height)
{
    this->width = width;
    this->height = height;
}

void VideoAppSink::eos()
{
}

QGst::FlowReturn VideoAppSink::newPreroll()
{
    QGst::SamplePtr sample = pullPreroll();

    QGst::MapInfo mapInfo;
    sample->buffer()->map(mapInfo, QGst::MapRead);

    QImage image((uchar*) mapInfo.data(), width, height, QImage::Format_ARGB32);
    sample->buffer()->unmap(mapInfo);

    emit newPrerollImage(image);

    return QGst::FlowOk;
}

QGst::FlowReturn VideoAppSink::newSample()
{
    // TODO: Allow to delay the buffer/frame here
    QGst::SamplePtr sample = pullSample();

    QGst::MapInfo mapInfo;
    sample->buffer()->map(mapInfo, QGst::MapRead);

    QImage image((uchar*) mapInfo.data(), width, height, QImage::Format_ARGB32);
    sample->buffer()->unmap(mapInfo);

    emit newImage(image);

    return QGst::FlowOk;
}
