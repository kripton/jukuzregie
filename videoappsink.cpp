#include "videoappsink.h"

VideoAppSink::VideoAppSink(QObject *parent) :
    QObject(parent), QGst::Utils::ApplicationSink()
{
    delay = 0;

    connect(this, SIGNAL(queueBuffer(QImage*, int)), this, SLOT(doQueueBuffer(QImage*,int)), Qt::QueuedConnection);
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

    // delay doesn't make sense for Preroll => just emit it!
    emit newPrerollImage(image);

    return QGst::FlowOk;
}

QGst::FlowReturn VideoAppSink::newSample()
{
    // TODO: Allow to delay the buffer/frame here
    QGst::SamplePtr sample = pullSample();

    QGst::MapInfo mapInfo;
    sample->buffer()->map(mapInfo, QGst::MapRead);

    QImage* img = new QImage((uchar*) mapInfo.data(), 640, 360, QImage::Format_ARGB32);
    sample->buffer()->unmap(mapInfo);

    qDebug() << "VIDEO DELAY:" << delay << "ms";

    if (delay == 0)
    {
        emit queueBuffer(img, delay);
        //emit newImage(*img);
        //delete img;
        //bufferQueue.clear();
    }
    else
    {
        emit queueBuffer(img, delay);
    }

    return QGst::FlowOk;
}

void VideoAppSink::doQueueBuffer(QImage *image, int delay)
{
    bufferQueue.enqueue(image);
    QTimer::singleShot(delay, this, SLOT(emitOldestBuffer()));
}

void VideoAppSink::emitOldestBuffer()
{
    if (bufferQueue.length() == 0)
    {
        return;
    }

    QImage* img = bufferQueue.dequeue();
    emit newImage(*img);
    delete img;
}
