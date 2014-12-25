#ifndef VIDEOAPPSINK_H
#define VIDEOAPPSINK_H

#include <QObject>
#include <QImage>
#include <QColor>

#include <QGlib/Error>
#include <QGlib/Connect>
#include <QGst/Init>
#include <QGst/Bus>
#include <QGst/Pipeline>
#include <QGst/Parse>
#include <QGst/Message>
#include <QGst/Memory>
#include <QGst/Buffer>
#include <QGst/Utils/ApplicationSink>

class VideoAppSink : public QObject, public QGst::Utils::ApplicationSink
{
    Q_OBJECT

public:
    explicit VideoAppSink(QObject *parent = 0);

signals:
    void newImage(QImage image);

public slots:

protected:
    virtual void eos();
    virtual QGst::FlowReturn newSample();

private:
    QImage image2;
};

#endif // VIDEOAPPSINK_H
