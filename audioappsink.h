#ifndef AUDIOAPPSINK_H
#define AUDIOAPPSINK_H

#include <QObject>
#include <QTimer>

#include <QGst/Memory>
#include <QGst/Buffer>
#include <QGst/Utils/ApplicationSink>

class AudioAppSink : public QObject, public QGst::Utils::ApplicationSink
{
    Q_OBJECT

public:
    explicit AudioAppSink(QObject *parent = 0);
    int delay;

signals:
    void queueBuffer(QByteArray *data, int delay);
    void newAudioBuffer(QByteArray data);

public slots:

protected:
    virtual void eos();
    virtual QGst::FlowReturn newSample();

private slots:
    void doQueueBuffer(QByteArray *data, int delay);

public slots:
    void emitBuffer(QByteArray *data);
};

#endif // AUDIOAPPSINK_H
