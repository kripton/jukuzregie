#ifndef AUDIOAPPSRC_H
#define AUDIOAPPSRC_H

#include <QObject>

#include <QGlib/Error>
#include <QGlib/Connect>
#include <QGst/Init>
#include <QGst/Bus>
#include <QGst/Pipeline>
#include <QGst/Parse>
#include <QGst/Message>
#include <QGst/Memory>
#include <QGst/Buffer>
#include <QGst/Utils/ApplicationSource>

class AudioAppSrc : public QObject, public QGst::Utils::ApplicationSource
{
    Q_OBJECT
public:
    explicit AudioAppSrc(QObject *parent = 0);

    bool preAlloc;

    virtual void needData (uint length);
    virtual void enoughData();

private:
    QGst::BufferPtr buffer;
    QGst::MapInfo mapInfo;

signals:
    void sigNeedData(uint length, char* data);

public slots:
    void pushAudioBuffer();
    void pushAudioBuffer(QByteArray data);
};

#endif // AUDIOAPPSRC_H
