#include "cambox.h"
#include "ui_cambox.h"

CamBox::CamBox(QWidget *parent):
    QGroupBox(parent),
    ui(new Ui::CamBox)
{
    ui->setupUi(this);

    this->setAutoFillBackground(true);
    camOnline = false;

    connect(ui->opacitySlider, SIGNAL(valueChanged(int)), this, SLOT(opcatiyFaderChanged()));
    connect(ui->volumeSlider, SIGNAL(valueChanged(int)), this, SLOT(volumeFaderChanged()));
    connect(ui->MonitorPushButton, SIGNAL(toggled(bool)), this, SLOT(preListenButtonToggled(bool)));
    connect(ui->GOButton, SIGNAL(clicked()), this, SLOT(goButtonClicked()));

    this->setAlignment(Qt::AlignHCenter);

    ui->volumeSlider->setValue(0);
    ui->opacitySlider->setValue(0);
    ui->volumeSlider->setEnabled(false);
    ui->opacitySlider->setEnabled(false);
    ui->MonitorPushButton->setEnabled(false);
    ui->GOButton->setEnabled(false);

    ui->VideoBox->setScene(&scene);
    pixmapItem = 0;

    m_tcpsrc = new TcpAppSrc(this);

    m_videosink = new VideoAppSink(this);
    connect(m_videosink, SIGNAL(newImage(QImage)), this, SLOT(newVideoFrameFromSink(QImage)));

    m_audiosink = new AudioAppSink(this);
    connect(m_audiosink, SIGNAL(newAudioBuffer(QByteArray)), this, SLOT(newAudioBufferFromSink(QByteArray)));


    fadeTimer = new QTimer(this);
    connect(fadeTimer, SIGNAL(timeout()), this, SLOT(fadeTimeEvent()));

    // This is a bit awkward. Somehow, the title gets changed back
    // if we just call the SLOT here. So, use a single-shot timer
    QTimer::singleShot(200, this, SLOT(updateBackground()));
}

CamBox::~CamBox()
{
    if (pipeline) pipeline->setState(QGst::StateNull);
    delete ui;
}

bool CamBox::getPreListen()
{
    return ui->MonitorPushButton->isChecked();
}

qreal CamBox::getVolume()
{
    return ui->volumeSlider->value() / 1000.0;
}

bool CamBox::getCamOnline() {
    return camOnline;
}

QHash<QString, QString> CamBox::sourceInfo()
{
    QHash<QString, QString> info;

    info["online"] = QString("%1").arg(camOnline);
    info["name"] = name;
    info["audiobuffersize"] = QString("%1").arg(audioData.size());
    info["volume"] = QString("%1").arg(ui->volumeSlider->value() / 1000.0);
    info["opacity"] = QString("%1").arg(ui->opacitySlider->value() / 1000.0);

    return info;
}

void CamBox::opcatiyFaderChanged()
{
    if (ui->volSync->isChecked())
    {
        setVolume(ui->opacitySlider->value() / 1000.0);
        volumeFaderChanged();
    }
    updateBackground();
    emit newOpacity(ui->opacitySlider->value() / 1000.0);
}

void CamBox::volumeFaderChanged()
{
    updateBackground();
    emit newVolume(ui->volumeSlider->value() / 1000.0);
}

void CamBox::setVideoOpacity(qreal opacity, bool diff) {
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

void CamBox::setVolume(qreal volume, bool diff) {
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

void CamBox::fadeStart(qreal stepSize, qint16 interval)
{
    //qDebug() << "fadeStart" << stepSize << interval;
    if (stepSize == 0) {
        fadeTimer->stop();
        return;
    }
    if (fadeTimer->isActive()) fadeTimer->stop();
    fadeStepSize = stepSize;
    fadeTimer->start(interval);
}

void CamBox::setDumpDir(QString dir)
{
    dumpDir = dir;
}

void CamBox::newVideoFrameFromSink(QImage image)
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

void CamBox::newAudioBufferFromSink(QByteArray data)
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
        qWarning() << "AUDIO BUFFER OVERFLOW in camBox" << id << "Samples have been dropped";
        // The samples are not saved anywhere and are dropped. THIS IS AUDIBLE!
    }

    // Set the new value to the avarage of the old and the new value. This makes the display less twitchy
    ui->AudioMeterSliderL->setValue(((maxleft * 100) + ui->AudioMeterSliderL->value()) / 2);
    ui->AudioMeterSliderR->setValue(((maxright * 100) + ui->AudioMeterSliderR->value()) / 2);
}

void CamBox::fadeTimeEvent()
{
    //qDebug() << "fadeTimeEvent" << ui->opacitySlider->value();
    if ((fadeStepSize > 0) && ((ui->opacitySlider->value() / 1000.0 + fadeStepSize) >= 1.0)) {
        setVideoOpacity(1.0);
        fadeTimer->stop();
    } else if ((fadeStepSize < 0) && ((ui->opacitySlider->value() / 1000.0  + fadeStepSize) <= 0.0)) {
        setVideoOpacity(0.0);
        fadeTimer->stop();
    } else {
        setVideoOpacity(fadeStepSize, true);
    }
}

void CamBox::onBusMessage(const QGst::MessagePtr &message)
{
    qDebug() << "MESSAGE" << message->type() << message->typeName();
    switch (message->type()) {
    case QGst::MessageEos:
        camOnline = false;
        sourceOffline();
        break;
    case QGst::MessageError: //Some error occurred.
        qCritical() << message.staticCast<QGst::ErrorMessage>()->error();
        camOnline = false;
        sourceOffline();
        break;
    case QGst::MessageStateChanged:
        updateBackground(); // Change between Online/Buffering/Ready
        break;
    case QGst::MessageTag:
    {
        QGst::TagList taglist = message.staticCast<QGst::TagMessage>()->taglist();
        if (taglist.comment() != "")
        {
            name = taglist.comment();
        }
        break;
    }

    default:
        break;
    }
}

void CamBox::setPreListen(bool value)
{
    ui->MonitorPushButton->setChecked(value);
}

void CamBox::startCam(QHostAddress host, quint16 port, QString videocaps, QString audiocaps)
{
    qDebug() << "Starting stream from" << QString("%1:%2").arg(host.toString()).arg(port) << id;
    QString dumpFileName = QString("%1/%2_%3.mkv")
            .arg(dumpDir)
            .arg(QDateTime::currentDateTime().toString("yyyy-mm-dd_hh-mm-ss"))
            .arg(id);

    QString desc = QString("appsrc name=source !"
                   " decodebin name=decode ! queue ! videoconvert ! appsink name=videosink caps=\"%1\""
                   " decode. ! queue ! audioconvert ! appsink name=audiosink caps=\"%2\"")
            .arg(videocaps)
            .arg(audiocaps);

    qDebug() << "Pipeline:" << desc;
    pipeline = QGst::Parse::launch(desc).dynamicCast<QGst::Pipeline>();

    QGlib::connect(pipeline->bus(), "message", this, &CamBox::onBusMessage);
    pipeline->bus()->addSignalWatch();

    m_tcpsrc->setElement(pipeline->getElementByName("source"));
    m_tcpsrc->start(host.toString(), port, dumpFileName);

    m_videosink->setElement(pipeline->getElementByName("videosink"));

    m_audiosink->setElement(pipeline->getElementByName("audiosink"));

    // start playing
    pipeline->setState(QGst::StatePlaying);

    sourceOnline();
}

void CamBox::sourceOnline() {
    if (camOnline) return;

    ui->volumeSlider->setValue(0);
    ui->opacitySlider->setValue(0);
    ui->volumeSlider->setEnabled(true);
    ui->opacitySlider->setEnabled(true);
    ui->MonitorPushButton->setEnabled(true);
    ui->GOButton->setEnabled(true);

    camOnline = true;
    updateBackground();
}

void CamBox::sourceOffline() {
    if (camOnline) return;

    qDebug() << "SOURCE ID" << id << "LEFT US";

    audioData.clear();

    camOnline = false;
    name = "";
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

void CamBox::updateBackground() {
    QPalette p = this->palette();
    if (camOnline == true) {
        if (pipeline->currentState() != QGst::StatePlaying)
        {
            // Source online but pipeline not PLAYING ... (Still buffering input)
            p.setColor(QPalette::Window, Qt::yellow);
            this->setTitle(QString("%1 (%2) : Buffering").arg(id).arg(name));
        } else if (ui->opacitySlider->value() != 0 || ui->volumeSlider->value() != 0) {
            // Source online and ONAIR (RED)
            p.setColor(QPalette::Window, Qt::red);
            this->setTitle(QString("%1 (%2) : ON AIR").arg(id).arg(name));
        } else {
            // Source online but not ONAIR (PALE GREEN)
            p.setColor(QPalette::Window, QColor(180, 255, 180));
            this->setTitle(QString("%1 (%2) : Bereit").arg(id).arg(name));
        }
    } else {
        // Source offline (LIGHT GRAY)
        p.setColor(QPalette::Window, Qt::lightGray);
        this->setTitle(QString("%1 : Nicht verbunden").arg(id));
    }
    this->setPalette(p);
}

void CamBox::preListenButtonToggled(bool checked)
{
    emit newPreListen(checked);
}

void CamBox::goButtonClicked()
{
    emit fadeMeIn();
}
