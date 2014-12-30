#include "videoappsink.h"

VideoAppSink::VideoAppSink(QObject *parent) :
    QObject(parent), QGst::Utils::ApplicationSink()
{
}

void VideoAppSink::eos()
{
}

QGst::FlowReturn VideoAppSink::newPreroll()
{
    QGst::SamplePtr sample = pullPreroll();

    QGst::MapInfo mapInfo;
    sample->buffer()->map(mapInfo, QGst::MapRead);

    QImage image((uchar*) mapInfo.data(), 640, 360, QImage::Format_ARGB32);
    sample->buffer()->unmap(mapInfo);

    emit newPrerollImage(image);

    return QGst::FlowOk;
}

QGst::FlowReturn VideoAppSink::newSample()
{
    QGst::SamplePtr sample = pullSample();

    QGst::MapInfo mapInfo;
    sample->buffer()->map(mapInfo, QGst::MapRead);

    QImage image((uchar*) mapInfo.data(), 640, 360, QImage::Format_ARGB32);
    sample->buffer()->unmap(mapInfo);

    emit newImage(image);

    return QGst::FlowOk;
}
