#include "audioappsink.h"

AudioAppSink::AudioAppSink(QObject *parent) :
    QObject(parent)
{
    delay = 0;

    connect(this, SIGNAL(queueBuffer(QByteArray*,int)), this, SLOT(doQueueBuffer(QByteArray*,int)), Qt::QueuedConnection);
}

void AudioAppSink::eos()
{
}

QGst::FlowReturn AudioAppSink::newSample()
{
    // TODO: Allow to delay the buffer/frame here
    QGst::SamplePtr sample = pullSample();

    QGst::MapInfo mapInfo;
    sample->buffer()->map(mapInfo, QGst::MapRead);

    QByteArray* data = new QByteArray((char*)mapInfo.data(), mapInfo.size());
    sample->buffer()->unmap(mapInfo);

    qDebug() << "AUDIO DELAY:" << delay << "ms";

    if (delay == 0)
    {
        emit newAudioBuffer(*data);
        delete data;
    }
    else
    {
        emit queueBuffer(data, delay);
    }

    return QGst::FlowOk;
}

void AudioAppSink::doQueueBuffer(QByteArray* data, int delay)
{
    QTimer::singleShot(delay, this, SLOT(emitBuffer(data)));
}

void AudioAppSink::emitBuffer(QByteArray *data)
{
    emit newAudioBuffer(*data);
    delete data;
}
