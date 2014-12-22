#include "videoappsink.h"

VideoAppSink::VideoAppSink(QObject *parent) :
    QObject(parent), QGst::Utils::ApplicationSink()
{
}

void VideoAppSink::eos()
{
}

QGst::FlowReturn VideoAppSink::newSample()
{
    QGst::SamplePtr sample = pullSample();

    QGst::MapInfo mapInfo;
    sample->buffer()->map(mapInfo, QGst::MapRead);

    QImage* image = new QImage((uchar*) mapInfo.data(), 640,360, QImage::Format_ARGB32);
    sample->buffer()->unmap(mapInfo);

    emit newImage(image); // deletion of memory has to be handled in slot

    return QGst::FlowOk;
}
