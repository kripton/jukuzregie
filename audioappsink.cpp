#include "audioappsink.h"

AudioAppSink::AudioAppSink(QObject *parent) :
    QObject(parent)
{
}

void AudioAppSink::eos()
{
}

QGst::FlowReturn AudioAppSink::newSample()
{
    QGst::SamplePtr sample = pullSample();

    QGst::MapInfo mapInfo;
    sample->buffer()->map(mapInfo, QGst::MapRead);

    QByteArray data((char*)mapInfo.data(), mapInfo.size());
    emit newAudioBuffer(data);

    return QGst::FlowOk;
}
