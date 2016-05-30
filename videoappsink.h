#ifndef VIDEOAPPSINK_H
#define VIDEOAPPSINK_H

#include <QObject>
#include <QImage>
#include <QColor>

#include <QGst/Memory>
#include <QGst/Buffer>
#include <QGst/Utils/ApplicationSink>

class VideoAppSink : public QObject, public QGst::Utils::ApplicationSink
{
    Q_OBJECT

public:
    explicit VideoAppSink(QObject *parent = 0);
    void setDimensions(int width, int height);

signals:
    void newPrerollImage(QImage image);
    void newImage(QImage image);

public slots:

protected:
    virtual void eos();
    virtual QGst::FlowReturn newPreroll();
    virtual QGst::FlowReturn newSample();

private:
    QImage image2;

    int width;
    int height;
};

#endif // VIDEOAPPSINK_H
