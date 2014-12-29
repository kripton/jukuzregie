#ifndef VIDEOAPPSRC_H
#define VIDEOAPPSRC_H

#include <QObject>
#include <QByteArray>

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

class VideoAppSrc : public QObject, public QGst::Utils::ApplicationSource
{
    Q_OBJECT
public:
    explicit VideoAppSrc(QObject *parent = 0);

    virtual void needData (uint length);
    virtual void enoughData();

signals:
    void sigNeedData(uint length);

public slots:
    void pushVideoBuffer(QByteArray data);
};

#endif // VIDEOAPPSRC_H
