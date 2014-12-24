#include "audioappsrc.h"

AudioAppSrc::AudioAppSrc(QObject *parent) :
    QObject(parent), QGst::Utils::ApplicationSource()
{
    enableBlock(false);
    setStreamType(QGst::AppStreamTypeStream);
    setLive(true);
}

void AudioAppSrc::needData(uint length)
{
    //qDebug() << "SOURCE NEED DATA. Length:" << length;
    emit sigNeedData(length);
}

void AudioAppSrc::enoughData()
{
    qDebug() << "SOURCE ENOUGH DATA";
}

void AudioAppSrc::pushAudioBuffer(QByteArray data)
{
    //qDebug() << "PUSHING AUDIO BUFFER. Length:" << data.size();

    QGst::BufferPtr buf = QGst::Buffer::create(data.size());
    QGst::MapInfo mapInfo;
    buf->map(mapInfo, QGst::MapWrite);
    memcpy(mapInfo.data(), data.data(), data.size());
    buf->unmap(mapInfo);

    pushBuffer(buf);
}
