#ifndef CAMBOX_H
#define CAMBOX_H

#include <QGroupBox>
#include <QUrl>
#include <QTimer>
#include <QMutex>
#include <QWaitCondition>
#include <QImage>
#include <QHostAddress>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QQueue>

#include "tcpappsrc.h"
#include "videoappsink.h"
#include "audioappsink.h"

#include <QGst/Clock>
#include <QGst/Bin>
#include <QGst/Memory>
#include <QGst/Buffer>
#include <QGst/Utils/ApplicationSink>

namespace Ui {
class CamBox;
}

class CamBox : public QGroupBox
{
    Q_OBJECT

public:
    explicit CamBox(QWidget *parent = 0);
    ~CamBox();

    QString id;                             // The id of this cambox. Like "cam_02"
    QString name;                           // The name of this cambox. Like "BühneTotale" or "Backstage"
    bool getPreListen();                    // read-only public access to state of Pre-Listen button
    qreal getVolume();                      // read-only public access to the current volume
    bool getCamOnline();                    // read-only public access to camOnline
    QHash<QString, QString> sourceInfo();
    void* userData;

    QQueue<float> audioData;

signals:
    void fadeMeIn();                        // emitted for parent when the GO-Button is clicked
    void newPreListen(bool newState);       // emitted for parent when the state of the Pre-Listen value changed
    void newOpacity(qreal newOpacity);      // emitted for parent when the opacity changed
    void newVolume(qreal newVolume);        // emitted for parent when the volume changed

    void newVideoFrame(QImage image);

public slots:
    void setVideoOpacity(qreal opacity, bool diff = false);    // set the opacity slider to a new value or modifiy the current one
    void setVolume(qreal volume, bool diff = false);           // set the volume fader to a new value or modifiy the current one
    void setPreListen(bool value);          // write-only public access to state of Pre-Listen button
    void startCam(QHostAddress host, quint16 port, QString videocaps, QString audiocaps); // start playing from a source
    void fadeStart(qreal stepSize, qint16 interval); // Start a fade on opacity
    void setDumpDir(QString dir);           // Specify in which directory the incoming stream should be archived to

private slots:
    void newVideoFrameFromSink(QImage image);
    void newAudioBufferFromSink(QByteArray data);

private slots:
    void opcatiyFaderChanged();             // called when the opacity-fader got changed
    void volumeFaderChanged();              // called when the volume-fader got changed
    void sourceOnline();                    // UI cleanups/defaults after the source has come online
    void sourceOffline();                   // UI cleanups/defaults after the source has gone offline
    void updateBackground();                // Update the background color and title of this cambox
    void preListenButtonToggled(bool checked);// Internal slot to handle Pre-Listen button state changes
    void goButtonClicked();                 // Internal slot to handle GO-Button clicks
    void fadeTimeEvent();

private:
    Ui::CamBox *ui;
    bool camOnline;
    QGst::PipelinePtr pipeline;
    QString dumpDir;

    void onBusMessage(const QGst::MessagePtr & message);

    TcpAppSrc* m_tcpsrc;

    VideoAppSink* m_videosink;
    AudioAppSink* m_audiosink;

    // Preview window stuff
    QGraphicsScene scene;
    QGraphicsPixmapItem* pixmapItem;

    qreal fadeStepSize;
    QTimer* fadeTimer;
};

#endif // CAMBOX_H
