#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QDebug>
#include <QGroupBox>
#include <QFileDialog>
#include <QQueue>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QTimer>

#include <QGlib/Connect>
#include <QGst/Parse>
#include <QGst/Pipeline>
#include <QGst/Bus>
#include <QGst/Message>
#include <QGst/Query>
#include <QGst/Event>

#include "audioappsink.h"
#include "videoappsink.h"

namespace Ui {
class VideoPlayer;
}

class VideoPlayer : public QGroupBox
{
    Q_OBJECT

public:
    void* userData;
    QQueue<float> audioData;

    explicit VideoPlayer(QWidget *parent = 0);
    ~VideoPlayer();
    void init(QString videocaps, QString audiocaps);

    bool getPreListen();                    // read-only public access to state of Pre-Listen button
    qreal getVolume();                      // read-only public access to the current volume
    QGst::State getState();

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
    void fadeStart(qreal stepSize, qint16 interval); // Start a fade on opacity
    void audioDiscontOn();                  // Call this slot if a discontinuity in the audio stream has occured

private slots:
    void selectFolder();
    void newFileSelected(QString newFile);

    void onStateChanged();
    void onPositionChanged();
    void setPosition(int position);

    void play();
    void pause();
    void stop();

    void opcatiyFaderChanged();             // called when the opacity-fader got changed
    void volumeFaderChanged();              // called when the volume-fader got changed
    void sourceOnline();                    // UI cleanups/defaults after the source has come online
    void sourceOffline();                   // UI cleanups/defaults after the source has gone offline
    void updateBackground();                // Update the background color and title of this cambox
    void preListenButtonToggled(bool checked);// Internal slot to handle Pre-Listen button state changes
    void goButtonClicked();                 // Internal slot to handle GO-Button clicks
    void fadeTimeEvent();
    void newVideoFrameFromSink(QImage image);
    void newAudioBufferFromSink(QByteArray data);
    void audioPeakOn();
    void audioPeakOff();
    void audioDiscontOff();


private:
    Ui::VideoPlayer *ui;
    bool playerReady;
    QString currentDir;

    QString videocaps;
    QString audiocaps;

    QGst::PipelinePtr pipeline;

    VideoAppSink* m_videosink;
    AudioAppSink* m_audiosink;

    // Preview window stuff
    QGraphicsScene scene;
    QGraphicsPixmapItem* pixmapItem;

    qreal fadeStepSize;
    QTimer fadeTimer;

    QTimer audioPeakTimer;

    QPalette defaultPalette;
    QTimer audioDiscontTimer;

    QTimer m_positionTimer;

    QTime length() const;
    QTime position() const;
    void setPosition(const QTime & pos);
    void onBusMessage(const QGst::MessagePtr & message);
    void handlePipelineStateChange(const QGst::StateChangedMessagePtr & scm);
};

#endif // VIDEOPLAYER_H
