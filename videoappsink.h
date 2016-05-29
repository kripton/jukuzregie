#ifndef VIDEOAPPSINK_H
#define VIDEOAPPSINK_H

#include <QObject>
#include <QImage>
#include <QColor>
#include <QQueue>
#include <QTimer>

#include <QGst/Memory>
#include <QGst/Buffer>
#include <QGst/Utils/ApplicationSink>

class VideoAppSink : public QObject, public QGst::Utils::ApplicationSink
{
    Q_OBJECT

public:
    explicit VideoAppSink(QObject *parent = 0);
    int delay;

private:
    QQueue<QImage*> bufferQueue;

signals:
    void newPrerollImage(QImage image);
    void newImage(QImage image);
    void queueBuffer(QImage* image, int delay);

public slots:

protected:
    virtual void eos();
    virtual QGst::FlowReturn newPreroll();
    virtual QGst::FlowReturn newSample();

private slots:
    void doQueueBuffer(QImage* image, int delay);

public slots:
    void emitOldestBuffer();
};

#endif // VIDEOAPPSINK_H
