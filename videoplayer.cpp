#include "videoplayer.h"
#include "ui_videoplayer.h"

VideoPlayer::VideoPlayer(QWidget *parent) :
    MediaSourceBase(parent),
    ui(new Ui::VideoPlayer)
{
    ui->setupUi(this);
    MediaSourceBase::init(ui->AudioMeterSliderL, ui->AudioMeterSliderR, ui->audioLabel, ui->GOButton, ui->VideoBox, ui->opacitySlider, ui->fadeCheck, ui->MonitorPushButton, ui->volumeSlider, ui->volSync);

    id = "VideoPlayer";

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

    connect(&videoSink, SIGNAL(newPrerollImage(QImage)), this, SLOT(newVideoFrameFromSink(QImage)));

    connect(&positionTimer, SIGNAL(timeout()), this, SLOT(onPositionChanged()));
}

VideoPlayer::~VideoPlayer()
{
    delete ui;
}

void VideoPlayer::init(QString videocaps, QString audiocaps)
{
    this->videocaps = videocaps;
    this->audiocaps = audiocaps;
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

    onPositionChanged();

    QString desc = QString(" filesrc location=\"%1\" ! decodebin name=decode !"
                           " queue ! videorate ! videoscale ! videoconvert ! appsink name=videosink caps=\"%2\""
                           " decode. ! queue ! audioresample ! audioconvert ! appsink name=audiosink caps=\"%3\"")
            .arg(QString("%1/%2").arg(currentDir).arg(newFile))
            .arg(videocaps)
            .arg(audiocaps);

    qDebug() << "Pipeline:" << desc;
    pipeline = QGst::Parse::launch(desc).dynamicCast<QGst::Pipeline>();

    QGlib::connect(pipeline->bus(), "message", (MediaSourceBase*)this, &MediaSourceBase::onBusMessage);
    QGlib::connect(pipeline->bus(), "message", this, &VideoPlayer::onBusMessage);
    pipeline->bus()->addSignalWatch();

    videoSink.setElement(pipeline->getElementByName("videosink"));

    audioSink.setElement(pipeline->getElementByName("audiosink"));

    // Let the pipeline preroll to check for errors and such
    pipeline->setState(QGst::StatePaused);

    updateBackground();
}

void VideoPlayer::onBusMessage(const QGst::MessagePtr &message)
{
    qDebug() << "VIDEOPLAYER MESSAGE" << message->type() << message->typeName();
    switch (message->type()) {
    case QGst::MessageEos:
        pause();
        break;
    case QGst::MessageError: //Some error occurred.
        qCritical() << message.staticCast<QGst::ErrorMessage>()->error();
        sourceOffline();
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
        positionTimer.start(100);

        if (scm->oldState() == QGst::StateNull)
        {
            sourceOnline();
        }
        break;
    case QGst::StatePaused:
        //stop the timer when the pipeline pauses
        if(scm->oldState() == QGst::StatePlaying) {
            positionTimer.stop();
        }
        break;
    default:
        break;
    }

    onStateChanged();
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

    ui->opacitySlider->setEnabled((newState == QGst::StatePlaying) || (newState == QGst::StatePaused));
    ui->volumeSlider->setEnabled((newState == QGst::StatePlaying) || (newState == QGst::StatePaused));

    //if we are in Null state, call onPositionChanged() to restore
    //the position of the slider and the text on the label
    if (newState == QGst::StateNull) {
        onPositionChanged();
        ui->playButton->setEnabled(false);
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

    QGst::State state = QGst::StateNull;
    if (!pipeline.isNull())
    {
        state = pipeline->currentState();
    }

    if ((state != QGst::StateReady) && (state != QGst::StateNull))
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
    sourceOffline();
    ui->fileList->clearSelection();
    updateBackground();

    if (!pipeline.isNull()) {
        pipeline->setState(QGst::StateNull);

        //once the pipeline stops, the bus is flushed so we will
        //not receive any StateChangedMessage about this.
        //so, to inform the ui, we have to emit this signal manually.
        onStateChanged();
    }
}
