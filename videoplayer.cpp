#include "videoplayer.h"
#include "ui_videoplayer.h"

VideoPlayer::VideoPlayer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::VideoPlayer)
{
    ui->setupUi(this);

    setAutoFillBackground(true);
    playerReady = false;

    ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    ui->playButton->setIconSize(QSize(32, 32));
    connect(ui->playButton, SIGNAL(clicked()), this, SLOT(play()));

    ui->pauseButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
    ui->pauseButton->setIconSize(QSize(32, 32));
    connect(ui->pauseButton, SIGNAL(clicked()), this, SLOT(pause()));

    ui->stopButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    ui->stopButton->setIconSize(QSize(32, 32));
    connect(ui->stopButton, SIGNAL(clicked()), this, SLOT(stop()));

    ui->dirSelectButton->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    ui->dirSelectButton->setIconSize(QSize(32, 32));
    connect(ui->dirSelectButton, SIGNAL(clicked()), this, SLOT(selectFolder()));
    connect(ui->fileList, SIGNAL(currentTextChanged(QString)), this, SLOT(newFileSelected(QString)));

    connect(ui->positionSlider, SIGNAL(sliderMoved(int)), this, SLOT(setPosition(int)));

    connect(ui->opacitySlider, SIGNAL(valueChanged(int)), this, SLOT(opcatiyFaderChanged()));
    connect(ui->volumeSlider, SIGNAL(valueChanged(int)), this, SLOT(volumeFaderChanged()));
    connect(ui->MonitorPushButton, SIGNAL(toggled(bool)), this, SLOT(preListenButtonToggled(bool)));
    connect(ui->GOButton, SIGNAL(clicked()), this, SLOT(goButtonClicked()));

    ui->volumeSlider->setValue(0);
    ui->opacitySlider->setValue(0);
    ui->volumeSlider->setEnabled(false);
    ui->opacitySlider->setEnabled(false);
    ui->MonitorPushButton->setEnabled(false);
    ui->GOButton->setEnabled(false);

    ui->VideoBox->setScene(&scene);
    pixmapItem = 0;

    m_videosink = new VideoAppSink(this);
    connect(m_videosink, SIGNAL(newImage(QImage)), this, SLOT(newVideoFrameFromSink(QImage)));

    m_audiosink = new AudioAppSink(this);
    connect(m_audiosink, SIGNAL(newAudioBuffer(QByteArray)), this, SLOT(newAudioBufferFromSink(QByteArray)));

    m_audiosink = new AudioAppSink(this);
    connect(m_audiosink, SIGNAL(newAudioBuffer(QByteArray)), this, SLOT(newAudioBufferFromSink(QByteArray)));

    connect(&fadeTimer, SIGNAL(timeout()), this, SLOT(fadeTimeEvent()));

    connect(&audioPeakTimer, SIGNAL(timeout()), this, SLOT(audioPeakOff()));

    defaultPalette = ui->audioLabel->palette();
    connect(&audioDiscontTimer, SIGNAL(timeout()), this, SLOT(audioDiscontOff()));

    connect(&m_positionTimer, SIGNAL(timeout()), this, SLOT(onPositionChanged()));
}

VideoPlayer::~VideoPlayer()
{
    if (pipeline) pipeline->setState(QGst::StateNull);
    delete ui;
}

bool VideoPlayer::getPreListen()
{
    return ui->MonitorPushButton->isChecked();
}

qreal VideoPlayer::getVolume()
{
    return ui->volumeSlider->value() / 1000.0;
}

void VideoPlayer::init(QString videocaps, QString audiocaps)
{
    this->videocaps = videocaps;
    this->audiocaps = audiocaps;
}

void VideoPlayer::opcatiyFaderChanged()
{
    if (ui->volSync->isChecked())
    {
        setVolume(ui->opacitySlider->value() / 1000.0);
        volumeFaderChanged();
    }
    updateBackground();
    emit newOpacity(ui->opacitySlider->value() / 1000.0);
}

void VideoPlayer::volumeFaderChanged()
{
    updateBackground();
    emit newVolume(ui->volumeSlider->value() / 1000.0);
}

void VideoPlayer::setVideoOpacity(qreal opacity, bool diff) {
    qreal newValue;

    if (diff)
    {
        qreal currentValue = ui->opacitySlider->value() / 1000.0;
        newValue = currentValue + opacity;
    }
    else
    {
        newValue = opacity;
    }
    if (newValue > 1.0) newValue = 1.0;
    if (newValue < 0.0) newValue = 0.0;
    ui->opacitySlider->setValue(newValue * 1000);

    opcatiyFaderChanged();
}

void VideoPlayer::setVolume(qreal volume, bool diff) {
    qreal newValue;

    if (diff)
    {
        qreal currentValue = ui->volumeSlider->value() / 1000.0;
        newValue = currentValue + volume;
    }
    else
    {
        newValue = volume;
    }
    if (newValue > 1.0) newValue = 1.0;
    if (newValue < 0.0) newValue = 0.0;
    ui->volumeSlider->setValue(newValue * 1000);

    volumeFaderChanged();
}

void VideoPlayer::fadeStart(qreal stepSize, qint16 interval)
{
    if (stepSize == 0) {
        fadeTimer.stop();
        return;
    }

    if (ui->fadeCheck->isChecked())
    {
        // Legacy behavior: Fade
        fadeStepSize = stepSize;
        fadeTimer.start(interval);
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

void VideoPlayer::selectFolder()
{
    currentDir = QFileDialog::getExistingDirectory(this, "Select folder for videoplayer");
    QDir dirInfo(currentDir);
    ui->fileList->clear();
    foreach (QString item, dirInfo.entryList(QDir::Files | QDir::NoDotAndDotDot | QDir::Readable, QDir::Name))
    {
        ui->fileList->addItem(item);
    }
}

void VideoPlayer::newFileSelected(QString newFile)
{
    qDebug() << "NEW FILE:" << currentDir << newFile;

    if (!pipeline.isNull())
    {
        pipeline->setState(QGst::StateNull);
        pipeline.clear();
    }

    QString desc = QString(" filesrc location=\"%1\" ! decodebin name=decode !"
                           " queue ! videorate ! videoscale ! videoconvert ! appsink name=videosink caps=\"%2\""
                           " decode. ! queue ! audioresample ! audioconvert ! appsink name=audiosink caps=\"%3\"")
            .arg(QString("%1/%2").arg(currentDir).arg(newFile))
            .arg(videocaps)
            .arg(audiocaps);

    qDebug() << "Pipeline:" << desc;
    pipeline = QGst::Parse::launch(desc).dynamicCast<QGst::Pipeline>();

    QGlib::connect(pipeline->bus(), "message", this, &VideoPlayer::onBusMessage);
    pipeline->bus()->addSignalWatch();

    m_videosink->setElement(pipeline->getElementByName("videosink"));

    m_audiosink->setElement(pipeline->getElementByName("audiosink"));

    // Let the pipeline preroll to check for errors and such
    pipeline->setState(QGst::StatePaused);

    playerReady = true;
    updateBackground();
}

void VideoPlayer::newVideoFrameFromSink(QImage image)
{
    // display it in the preview
    QImage previewImage = image.scaled(320, 180, Qt::KeepAspectRatio, Qt::FastTransformation);
    if (pixmapItem == 0)
    {
        pixmapItem = scene.addPixmap(QPixmap::fromImage(previewImage));
    }
    pixmapItem->setPixmap(QPixmap::fromImage(previewImage));

    // Emit it so that the mainWindow can pick it up for the main image
    emit newVideoFrame(image);
}

void VideoPlayer::newAudioBufferFromSink(QByteArray data)
{
    //qDebug() << "NEW BUFFER FROM SINK. SIZE:" << data.size();

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
        qWarning() << "AUDIO BUFFER OVERFLOW in VideoPlayer. Samples have been dropped";
        audioDiscontOn();
    }

    // Set the new value to the avarage of the old and the new value. This makes the display less twitchy
    ui->AudioMeterSliderL->setValue(((maxleft * 100) + ui->AudioMeterSliderL->value()) / 2);
    ui->AudioMeterSliderR->setValue(((maxright * 100) + ui->AudioMeterSliderR->value()) / 2);

    if ((maxleft >= 1.0) || (maxright >= 1.0))
    {
        audioPeakOn();
    }
}

void VideoPlayer::audioPeakOn()
{
    QFont font = ui->audioLabel->font();
    font.setBold(true);
    ui->audioLabel->setFont(font);
    audioPeakTimer.start(1000);
}

void VideoPlayer::audioPeakOff()
{
    QFont font = ui->audioLabel->font();
    font.setBold(false);
    ui->audioLabel->setFont(font);
    audioPeakTimer.stop();
}

void VideoPlayer::audioDiscontOn()
{
    QPalette pal = ui->audioLabel->palette();
    pal.setColor(QPalette::WindowText, Qt::white);
    ui->audioLabel->setPalette(pal);
    audioDiscontTimer.start(1000);
}

void VideoPlayer::audioDiscontOff()
{
    ui->audioLabel->setPalette(defaultPalette);
    audioDiscontTimer.stop();
}

void VideoPlayer::onBusMessage(const QGst::MessagePtr &message)
{
    qDebug() << "VIDEOPLAYER MESSAGE" << message->type() << message->typeName();
    switch (message->type()) {
    case QGst::MessageEos:
        break;
    case QGst::MessageError: //Some error occurred.
        qCritical() << message.staticCast<QGst::ErrorMessage>()->error();
        break;
    case QGst::MessageStateChanged:
        if (message->source() == pipeline) {
            handlePipelineStateChange(message.staticCast<QGst::StateChangedMessage>());
        }
        break;
    default:
        break;
    }
}

void VideoPlayer::handlePipelineStateChange(const QGst::StateChangedMessagePtr & scm)
{
    switch (scm->newState()) {
    case QGst::StatePlaying:
        //start the timer when the pipeline starts playing
        m_positionTimer.start(100);
        break;
    case QGst::StatePaused:
        //stop the timer when the pipeline pauses
        if(scm->oldState() == QGst::StatePlaying) {
            m_positionTimer.stop();
        }
        break;
    default:
        break;
    }

    onStateChanged();
}

void VideoPlayer::fadeTimeEvent()
{
    //qDebug() << "fadeTimeEvent" << ui->opacitySlider->value();
    if ((fadeStepSize > 0) && ((ui->opacitySlider->value() / 1000.0 + fadeStepSize) >= 1.0)) {
        setVideoOpacity(1.0);
        fadeTimer.stop();
    } else if ((fadeStepSize < 0) && ((ui->opacitySlider->value() / 1000.0  + fadeStepSize) <= 0.0)) {
        setVideoOpacity(0.0);
        fadeTimer.stop();
    } else {
        setVideoOpacity(fadeStepSize, true);
    }
}

void VideoPlayer::setPreListen(bool value)
{
    ui->MonitorPushButton->setChecked(value);
}

void VideoPlayer::sourceOnline() {
    if (playerReady) return;

    ui->volumeSlider->setValue(0);
    ui->opacitySlider->setValue(0);
    ui->volumeSlider->setEnabled(true);
    ui->opacitySlider->setEnabled(true);
    ui->MonitorPushButton->setEnabled(true);
    ui->GOButton->setEnabled(true);

    playerReady = true;
    updateBackground();
}

void VideoPlayer::sourceOffline() {
    if (playerReady) return;

    qDebug() << "VIDEOPLAYER NO LONGER READY";

    audioData.clear();

    updateBackground();

    ui->AudioMeterSliderL->setValue(0);
    ui->AudioMeterSliderR->setValue(0);
    ui->volumeSlider->setValue(0);
    ui->opacitySlider->setValue(0);
    ui->volumeSlider->setEnabled(false);
    ui->opacitySlider->setEnabled(false);
    ui->MonitorPushButton->setEnabled(false);
    ui->GOButton->setEnabled(false);
}

void VideoPlayer::updateBackground() {
    QPalette p = this->palette();
    if (playerReady == true) {
        if (pipeline->currentState() != QGst::StatePlaying)
        {
            // Player loaded file not not playing
            p.setColor(QPalette::Window, Qt::yellow);
            //this->setTitle(QString("%1 (%2) : Ready").arg(id).arg(name));
            ui->GOButton->setEnabled(false);
        } else if (ui->opacitySlider->value() != 0 || ui->volumeSlider->value() != 0) {
            // Source online and ONAIR (RED)
            p.setColor(QPalette::Window, Qt::red);
            //this->setTitle(QString("%1 (%2) : ON AIR").arg(id).arg(name));
        } else {
            // Source online but not ONAIR (PALE GREEN)
            p.setColor(QPalette::Window, QColor(180, 255, 180));
            //this->setTitle(QString("%1 (%2) : Bereit").arg(id).arg(name));
            ui->GOButton->setEnabled(true);
        }
    } else {
        // Source offline (LIGHT GRAY)
        p.setColor(QPalette::Window, Qt::lightGray);
        //this->setTitle(QString("%1 : Nicht verbunden").arg(id));
    }
    this->setPalette(p);
}

void VideoPlayer::preListenButtonToggled(bool checked)
{
    emit newPreListen(checked);
}

void VideoPlayer::goButtonClicked()
{
    emit fadeMeIn();
}

void VideoPlayer::onStateChanged()
{
    QGst::State newState = QGst::StateNull;
    if (!pipeline.isNull())
    {
        newState = pipeline->currentState();
    }
    ui->playButton->setEnabled(newState != QGst::StatePlaying);
    ui->pauseButton->setEnabled(newState == QGst::StatePlaying);
    ui->stopButton->setEnabled(newState != QGst::StateNull);
    ui->positionSlider->setEnabled(newState != QGst::StateNull);

    //if we are in Null state, call onPositionChanged() to restore
    //the position of the slider and the text on the label
    if (newState == QGst::StateNull) {
        onPositionChanged();
    }

    updateBackground();
}

QTime VideoPlayer::length() const
{
    if (pipeline) {
        //here we query the pipeline about the content's duration
        //and we request that the result is returned in time format
        QGst::DurationQueryPtr query = QGst::DurationQuery::create(QGst::FormatTime);
        pipeline->query(query);
        return QGst::ClockTime(query->duration()).toTime();
    } else {
        return QTime(0,0);
    }
}

QTime VideoPlayer::position() const
{
    if (pipeline) {
        //here we query the pipeline about its position
        //and we request that the result is returned in time format
        QGst::PositionQueryPtr query = QGst::PositionQuery::create(QGst::FormatTime);
        pipeline->query(query);
        return QGst::ClockTime(query->position()).toTime();
    } else {
        return QTime(0,0);
    }
}

void VideoPlayer::setPosition(const QTime & pos)
{
    QGst::SeekEventPtr evt = QGst::SeekEvent::create(
        1.0, QGst::FormatTime, QGst::SeekFlagFlush,
        QGst::SeekTypeSet, QGst::ClockTime::fromTime(pos),
        QGst::SeekTypeNone, QGst::ClockTime::None
    );

    pipeline->sendEvent(evt);
}

void VideoPlayer::onPositionChanged()
{
    QTime len(0,0);
    QTime curpos(0,0);

    if ((pipeline->currentState() != QGst::StateReady) && (pipeline->currentState() != QGst::StateNull))
    {
        len = length();
        curpos = position();
    }

    ui->positionLabel->setText(curpos.toString("hh:mm:ss.zzz")
                                        + "/" +
                             len.toString("hh:mm:ss.zzz"));

    if (len != QTime(0,0)) {
        ui->positionSlider->setValue(curpos.msecsTo(QTime(0,0)) * 1000 / len.msecsTo(QTime(0,0)));
    } else {
        ui->positionSlider->setValue(0);
    }

    if (curpos != QTime(0,0)) {
        ui->positionLabel->setEnabled(true);
        ui->positionSlider->setEnabled(true);
    }
}

/* Called when the user changes the slider's position */
void VideoPlayer::setPosition(int value)
{
    uint len = -length().msecsTo(QTime(0,0));
    if (len != 0 && value > 0) {
        QTime pos(0,0);
        pos = pos.addMSecs(len * (value / 1000.0));
        setPosition(pos);
    }
}

void VideoPlayer::play()
{
    if (pipeline) {
        pipeline->setState(QGst::StatePlaying);
    }
}

void VideoPlayer::pause()
{
    if (pipeline) {
        pipeline->setState(QGst::StatePaused);
    }
}

void VideoPlayer::stop()
{
    playerReady = false;
    sourceOffline();
    ui->fileList->clearSelection();
    updateBackground();

    if (pipeline) {
        pipeline->setState(QGst::StateNull);

        //once the pipeline stops, the bus is flushed so we will
        //not receive any StateChangedMessage about this.
        //so, to inform the ui, we have to emit this signal manually.
        onStateChanged();
    }
}
