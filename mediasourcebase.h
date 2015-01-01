#ifndef MEDIASOURCEBASE_H
#define MEDIASOURCEBASE_H

#include <QObject>
#include <QDebug>
#include <QGroupBox>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QQueue>
#include <QGraphicsView>
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

class MediaSourceBase : public QGroupBox
{
    Q_OBJECT

public:
    /// FIELDS ///
    void* userData;

    /// PROPERTIES ///
    QString getId();
    void setId(QString id);
    QString getName();
    int getQueuedSamplesCount();
    bool getMonitor();
    qreal getVolume();
    QGst::State getState();
    virtual QHash<QString, QString> getSourceInfo();

    /// METHODS ///
    explicit MediaSourceBase(QWidget *parent);
    ~MediaSourceBase();

    void init(QSlider* leftMeterSlider, QSlider* rightMeterSlider, QLabel* meterLabel,
            QPushButton* goButton, QGraphicsView* previewGraphicsView,
            QSlider* opacitySlider, QCheckBox* fadeCheckBox, QPushButton* monitorButton,
            QSlider* volumeSlider, QCheckBox* syncVolToOpacityCheckBox);

    float dequeueSample();
    void clearQueuedSamples();

    void onBusMessage(const QGst::MessagePtr & message);

private:
    /// POINTERS TO UI ELEMENTS ///
    QSlider* leftMeterSlider;
    QSlider* rightMeterSlider;
    QLabel* meterLabel;
    QPushButton* goButton;
    QGraphicsView* previewGraphicsView;
    QSlider* opacitySlider;
    QCheckBox* fadeCheckBox;
    QPushButton* monitorButton;
    QSlider* volumeSlider;
    QCheckBox* syncVolToOpacityCheckBox;

protected:
    /// FIELDS ///
    QString id;
    QString name;
    QGst::PipelinePtr pipeline;
    QQueue<float> audioData;

    VideoAppSink videoSink;
    AudioAppSink audioSink;

    QGraphicsScene scene;
    QGraphicsPixmapItem* pixmapItem;

    qreal fadeStepSize;
    QTimer fadeStepTimer;

    QTimer audioPeakTimer;
    QPalette defaultPalette;
    QTimer audioDiscontTimer;

signals:
    void goButtonClicked();                     // emitted for parent when the GO-Button is clicked
    void preListenChanged(bool newState);       // emitted for parent when the state of the Pre-Listen value changed
    void opacityChanged(qreal newOpacity);      // emitted for parent when the opacity changed
    void volumeChanged(qreal newVolume);        // emitted for parent when the volume changed
    void newVideoFrame(QImage image);
    void pipelineEOS();
    void pipelineError(QString error, QString debugMsg);
    void pipelineNewState(QGst::State newState);

public slots:
    void setVideoOpacity(qreal opacity, bool diff = false);  // set the opacity slider to a new, absolute value or modifiy the current one in a relative manner
    void setVolume(qreal volume, bool diff = false);         // set the volume fader to a new, absolute value or modifiy the current one in a relative manner
    void setPreListen(bool value);                           // write-only public access to state of Pre-Listen button
    void fadeStart(qreal stepSize, qint16 interval);         // Start a fade on opacity
    void audioDiscontOn();                                   // Call this slot if a discontinuity in the audio stream has been detected

protected slots:
    void opcatiyFaderChanged();                    // called when the opacity-fader got changed
    void volumeFaderChanged();                     // called when the volume-fader got changed
    void sourceOnline();                           // UI cleanups/defaults after the source has come online
    void sourceOffline();                          // UI cleanups/defaults after the source has gone offline
    void updateBackground();                       // Update the background color and title of the groupBox
    void fadeTimeEvent();                          // fadeStepTimer had a timeout
    void newVideoFrameFromSink(QImage image);      // handle images provided by the VideoAppSink
    void newAudioBufferFromSink(QByteArray data);  // handle audio buffers provided by the AudioAppSink
    void audioClipOn();                            // call whenever an audio clip has been detected
    void audioClipOff();                           // call when the audio clip is over to reset the UI
    void audioDiscontOff();                        // call when the audio discont is over to reset the UI
};

#endif // MEDIASOURCEBASE_H
