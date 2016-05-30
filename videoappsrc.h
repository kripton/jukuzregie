#ifndef VIDEOAPPSRC_H
#define VIDEOAPPSRC_H

#include <QObject>
#include <QByteArray>

#include <QGst/Memory>
#include <QGst/Buffer>
#include <QGst/Utils/ApplicationSource>

class VideoAppSrc : public QObject, public QGst::Utils::ApplicationSource
{
    Q_OBJECT
public:
    explicit VideoAppSrc(QObject *parent = 0);
    void setDimensions(int width, int height);

    virtual void needData (uint length);
    virtual void enoughData();

private:
    QGst::BufferPtr buffer;
    QGst::MapInfo mapInfo;

    int width;
    int height;

signals:
    void sigNeedData(uint length, char* data);

public slots:
    void pushVideoBuffer();
};

#endif // VIDEOAPPSRC_H
