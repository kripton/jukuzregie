#ifndef AUDIOAPPSINK_H
#define AUDIOAPPSINK_H

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
#include <QGst/Utils/ApplicationSink>

class AudioAppSink : public QObject, public QGst::Utils::ApplicationSink
{
    Q_OBJECT

public:
    explicit AudioAppSink(QObject *parent = 0);

signals:
    void newAudioBuffer(QByteArray data);

public slots:

protected:
    virtual void eos();
    virtual QGst::FlowReturn newSample();

private:
};

#endif // AUDIOAPPSINK_H
