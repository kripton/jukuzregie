#include "mediasourcebase.h"

/// PUBLIC METHODS ///

QString MediaSourceBase::getId()
{
    return id;
}

void MediaSourceBase::setId(QString id)
{
    this->id = id;
}

QString MediaSourceBase::getName()
{
    return name;
}

int MediaSourceBase::getQueuedSamplesCount()
{
    return audioData.size();
}

bool MediaSourceBase::getMonitor()
{
    return monitorButton->isChecked();
}

qreal MediaSourceBase::getVolume()
{
    return (float)volumeSlider->value() / (float)volumeSlider->maximum();
}

QGst::State MediaSourceBase::getState()
{
    if (pipeline.isNull())
    {
        return QGst::StateNull;
    }
    return pipeline->currentState();
}

QHash<QString, QString> MediaSourceBase::getSourceInfo()
{
    QHash<QString, QString> info;

    info["online"] = QString("%1").arg(getState());
    info["name"] = name;
    info["audiobuffersize"] = QString("%1").arg(audioData.size());
    info["volume"] = QString("%1").arg(volumeSlider->value() / 1000.0);
    info["opacity"] = QString("%1").arg(opacitySlider->value() / 1000.0);

    return info;
}

MediaSourceBase::MediaSourceBase(QWidget *parent = 0) :
    QGroupBox(parent)
{
}

MediaSourceBase::~MediaSourceBase()
{
    if (!pipeline.isNull())
    {
        pipeline->setState(QGst::StateNull);
    }
}

void MediaSourceBase::init(QSlider *leftMeterSlider, QSlider *rightMeterSlider, QLabel *meterLabel,
                           QPushButton *goButton, QGraphicsView *previewGraphicsView,
                           QSlider *opacitySlider, QCheckBox *fadeCheckBox, QPushButton *monitorButton,
                           QSlider *volumeSlider, QCheckBox *syncVolToOpacityCheckBox)
{
    this->leftMeterSlider = leftMeterSlider;
    this->rightMeterSlider = rightMeterSlider;
    this->meterLabel = meterLabel;
    this->goButton = goButton;
    this->previewGraphicsView = previewGraphicsView;
    this->opacitySlider = opacitySlider;
    this->fadeCheckBox = fadeCheckBox;
    this->monitorButton = monitorButton;
    this->volumeSlider = volumeSlider;
    this->syncVolToOpacityCheckBox = syncVolToOpacityCheckBox;

    volumeSlider->setValue(0);
    opacitySlider->setValue(0);
    volumeSlider->setEnabled(false);
    opacitySlider->setEnabled(false);
    monitorButton->setEnabled(false);
    goButton->setEnabled(false);

    setAutoFillBackground(true);
    setAlignment(Qt::AlignHCenter);

    connect(opacitySlider, SIGNAL(valueChanged(int)), this, SLOT(opcatiyFaderChanged()));
    connect(volumeSlider, SIGNAL(valueChanged(int)), this, SLOT(volumeFaderChanged()));
    connect(goButton, SIGNAL(clicked()), this, SLOT(goButtonClicked()));

    previewGraphicsView->setScene(&scene);
    previewPixmapItem = scene.addPixmap(QPixmap());


    connect(&videoSink, SIGNAL(newImage(QImage)), this, SLOT(newVideoFrameFromSink(QImage)));
    connect(&audioSink, SIGNAL(newAudioBuffer(QByteArray)), this, SLOT(newAudioBufferFromSink(QByteArray)));
    connect(&fadeStepTimer, SIGNAL(timeout()), this, SLOT(fadeTimeEvent()));
    connect(&audioPeakTimer, SIGNAL(timeout()), this, SLOT(audioClipOff()));

    defaultPalette = meterLabel->palette();
    connect(&audioDiscontTimer, SIGNAL(timeout()), this, SLOT(audioDiscontOff()));
}

float MediaSourceBase::dequeueSample()
{
    return audioData.dequeue();
}

void MediaSourceBase::clearQueuedSamples()
{
    audioData.clear();
}

void MediaSourceBase::onBusMessage(const QGst::MessagePtr &message)
{
    qDebug() << "SOURCEPIPELINE" << id << "MESSAGE" << message->type() << message->typeName();
    switch (message->type()) {
    case QGst::MessageEos:
        sourceOffline();
        emit pipelineEOS();
        break;
    case QGst::MessageError:
        qCritical() << "SOURCEPIPELINE" << id << "ERROR:" << message.staticCast<QGst::ErrorMessage>()->error() << message.staticCast<QGst::ErrorMessage>()->debugMessage();
        sourceOffline();
        emit pipelineError(message.staticCast<QGst::ErrorMessage>()->error().message(), message.staticCast<QGst::ErrorMessage>()->debugMessage());
        break;
    case QGst::MessageStateChanged:
        qDebug() << "NEWSTATE:" << message.staticCast<QGst::StateChangedMessage>()->newState();
        emit pipelineNewState(message.staticCast<QGst::StateChangedMessage>()->newState());
        break;
    default:
        break;
    }

    updateBackground();
}


/// PUBLIC SLOTS ///

void MediaSourceBase::setVideoOpacity(qreal opacity, bool diff)
{
    qreal newValue;

    if (diff)
    {
        qreal currentValue = opacitySlider->value() / 1000.0;
        newValue = currentValue + opacity;
    }
    else
    {
        newValue = opacity;
    }
    if (newValue > 1.0) newValue = 1.0;
    if (newValue < 0.0) newValue = 0.0;
    opacitySlider->setValue(newValue * 1000);

    opcatiyFaderChanged();
}

void MediaSourceBase::setVolume(qreal volume, bool diff)
{
    qreal newValue;

    if (diff)
    {
        qreal currentValue = volumeSlider->value() / 1000.0;
        newValue = currentValue + volume;
    }
    else
    {
        newValue = volume;
    }
    if (newValue > 1.0) newValue = 1.0;
    if (newValue < 0.0) newValue = 0.0;
    volumeSlider->setValue(newValue * 1000);

    volumeFaderChanged();
}

void MediaSourceBase::setPreListen(bool value)
{
    monitorButton->setChecked(value);
}

void MediaSourceBase::fadeStart(qreal stepSize, qint16 interval)
{
    if (stepSize == 0) {
        fadeStepTimer.stop();
        return;
    }

    if (fadeCheckBox->isChecked())
    {
        // Legacy behavior: Fade
        fadeStepSize = stepSize;
        fadeStepTimer.start(interval);
    }
    else
    {
        if (stepSize > 0)
        {
            setVideoOpacity(1.0);
        }
        else
        {
            setVideoOpacity(0.0);
        }
    }
}

void MediaSourceBase::audioDiscontOn()
{
    QPalette pal = meterLabel->palette();
    pal.setColor(QPalette::WindowText, Qt::white);
    meterLabel->setPalette(pal);
    audioDiscontTimer.start(1000);
}

void MediaSourceBase::opcatiyFaderChanged()
{
    if (syncVolToOpacityCheckBox->isChecked())
    {
        setVolume(opacitySlider->value() / 1000.0);
        volumeFaderChanged();
    }
    updateBackground();
    emit opacityChanged(opacitySlider->value() / 1000.0);
}

void MediaSourceBase::volumeFaderChanged()
{
    updateBackground();
    emit volumeChanged(volumeSlider->value() / 1000.0);
}

void MediaSourceBase::goButtonClicked()
{
    emit fadeMeIn(true);
}

void MediaSourceBase::sourceOnline()
{
    qDebug() << "SOURCE" << id << "IS NOW ONLINE";

    volumeSlider->setValue(0);
    opacitySlider->setValue(0);
    volumeSlider->setEnabled(true);
    opacitySlider->setEnabled(true);
    monitorButton->setEnabled(true);
    goButton->setEnabled(true);

    updateBackground();
}

void MediaSourceBase::sourceOffline()
{
    qDebug() << "SOURCE" << id << "LEFT US";

    leftMeterSlider->setValue(0);
    rightMeterSlider->setValue(0);
    volumeSlider->setValue(0);
    opacitySlider->setValue(0);
    volumeSlider->setEnabled(false);
    opacitySlider->setEnabled(false);
    monitorButton->setEnabled(false);
    goButton->setEnabled(false);

    audioData.clear();

    name = "";

    updateBackground();

}

void MediaSourceBase::updateBackground()
{
    //qDebug() << "BASE UPDATEBACKGROUND" << id << "Current pipeline state:" << getState();

    QPalette p = palette();

    switch (getState())
    {
        case QGst::StateReady:
        case QGst::StatePaused:
            // Source online but pipeline not PLAYING ... (Still buffering input)
            p.setColor(QPalette::Window, Qt::yellow);
            goButton->setEnabled(false);
            break;
        case QGst::StatePlaying:
            if ((opacitySlider->value() != 0) || (volumeSlider->value() != 0)) {
                // Source online and ONAIR (RED)
                p.setColor(QPalette::Window, Qt::red);
                goButton->setEnabled(true);
            } else {
                // Source online but not ONAIR (PALE GREEN)
                p.setColor(QPalette::Window, QColor(180, 255, 180));
                goButton->setEnabled(true);
            }
            break;
        case QGst::StateNull:
            // Source offline (LIGHT GRAY)
            p.setColor(QPalette::Window, Qt::lightGray);
            break;

        default:
            break;
    }

    setPalette(p);
}

void MediaSourceBase::fadeTimeEvent()
{
    if ((fadeStepSize > 0) && ((opacitySlider->value() / 1000.0 + fadeStepSize) >= 1.0)) {
        setVideoOpacity(1.0);
        fadeStepTimer.stop();
    } else if ((fadeStepSize < 0) && ((opacitySlider->value() / 1000.0  + fadeStepSize) <= 0.0)) {
        setVideoOpacity(0.0);
        fadeStepTimer.stop();
    } else {
        setVideoOpacity(fadeStepSize, true);
    }
}

void MediaSourceBase::newVideoFrameFromSink(QImage image)
{
    //qDebug() << "MEDIASOURCE" << id << "NEW VIDEO DRAME FROM SINK.";

    // display it in the preview
    QImage previewImage = image.scaled(320, 180, Qt::KeepAspectRatio, Qt::FastTransformation);
    previewPixmapItem->setPixmap(QPixmap::fromImage(previewImage));

    // Emit it so that the mainWindow can pick it up for the main image
    emit newVideoFrame(image);
}

void MediaSourceBase::newAudioBufferFromSink(QByteArray data)
{
    //qDebug() << "MEDIASOURCE" << id << "NEW AUDIO BUFFER FROM SINK. SIZE:" << data.size();

    float* buffer = (float*)data.data();

    // Most simple peak calculation
    float maxleft = 0.0;
    float maxright = 0.0;

    if (audioData.size() < 131070)
    {
        // No use of omp parallel for here since the .enqueue() needs to be done in the correct order!
        for (int i = 0; i < (data.size() / (int)sizeof(float)); i = i + 2)
        {
            maxleft = std::max(maxleft, buffer[i]);
            maxright = std::max(maxright, buffer[i + 1]);
            audioData.enqueue(buffer[i]);
            audioData.enqueue(buffer[i+1]);
        }
    }
    else
    {
        // The samples are not saved anywhere and are dropped. THIS IS AUDIBLE!
        qWarning() << "AUDIO BUFFER OVERFLOW source" << getId() << "Samples have been dropped";
        audioDiscontOn();
    }

    // Set the new value to the avarage of the old and the new value. This makes the display less twitchy
    leftMeterSlider->setValue(((maxleft * 100) + leftMeterSlider->value()) / 2);
    rightMeterSlider->setValue(((maxright * 100) + rightMeterSlider->value()) / 2);

    if ((maxleft >= 1.0) || (maxright >= 1.0))
    {
        audioClipOn();
    }
}

void MediaSourceBase::audioClipOn()
{
    QFont font = meterLabel->font();
    font.setBold(true);
    meterLabel->setFont(font);
    audioPeakTimer.start(1000);
}

void MediaSourceBase::audioClipOff()
{
    QFont font = meterLabel->font();
    font.setBold(false);
    meterLabel->setFont(font);
    audioPeakTimer.stop();
}

void MediaSourceBase::audioDiscontOff()
{
    meterLabel->setPalette(defaultPalette);
    audioDiscontTimer.stop();
}
