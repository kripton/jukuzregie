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

    fadeTimer = new QTimer(this);
    connect(fadeTimer, SIGNAL(timeout()), this, SLOT(fadeTimeEvent()));

    // This is a bit awkward. Somehow, the title gets changed back
    // if we just call the SLOT here. So, use a single-shot timer
    QTimer::singleShot(200, this, SLOT(updateBackground()));
}

CamBox::~CamBox()
{
    delete ui;
}

bool CamBox::getPreListen()
{
    return ui->MonitorPushButton->isChecked();
}

bool CamBox::getCamOnline() {
    return camOnline;
}

QHash<QString, QString> CamBox::sourceInfo()
{
    QHash<QString, QString> info;

    info["online"] = QString("%1").arg(camOnline);

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
// TODO
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

void CamBox::setPreListen(bool value)
{
    ui->MonitorPushButton->setChecked(value);
}

QGst::BinPtr CamBox::startCam(QHostAddress host, quint16 port, QGst::CapsPtr videocaps, QGst::CapsPtr audiocaps)
{
    qDebug() << "Starting stream from" << QString("%1:%2").arg(host.toString()).arg(port) << name;
    QString dumpFileName = QString("/home/kripton/streaming/dump/%1.ogg").arg(name);

    // TODO: Dump stream to disk
    /*
    QString desc = QString("uridecodebin uri=rtsp://%1:%2/test name=decode ! "
                           "queue ! videoscale ! %3 ! tee name=video ! queue ! xvimagesink name=previewsink "
                           "video. ! queue name=voutqueue "
                           "decode. ! queue ! audioconvert ! level name=level_%4 ! audioconvert ! %5 ! tee name=audio ! queue name=a2outqueue "
                           "audio. ! queue name=a2prelistenqueue" )
            .arg(host.toString())
            .arg(port)
            .arg(videocaps->toString())
            .arg(name)
            .arg(audiocaps->toString())
            .arg(dumpFileName);
    */


    QString desc = QString("tcpclientsrc host=%1 port=%2 ! gdpdepay ! tee name=stream ! decodebin name=decode ! "
                           "queue ! videoscale ! %3 ! tee name=video ! queue ! xvimagesink name=previewsink "
                           "video. ! queue name=voutqueue "
                           "decode. ! queue ! audioconvert ! level name=level_%4 ! audioconvert ! %5 ! tee name=audio ! queue name=a2outqueue "
                           "audio. ! queue name=a2prelistenqueue "
                           "stream. ! queue ! filesink location=\"%6\"")
            .arg(host.toString())
            .arg(port)
            .arg(videocaps->toString())
            .arg(name)
            .arg(audiocaps->toString())
            .arg(dumpFileName);

    /*QString desc = QString("videotestsrc ! videoscale ! %1 ! tee name=t ! queue ! xvimagesink name=previewsink "
                           "t. ! queue name=voutqueue ").arg(videocaps->toString());*/
    qDebug() << desc;
    QGst::BinPtr bin = QGst::Bin::fromDescription(desc, QGst::Bin::NoGhost);

    ui->VideoWidget->setVideoSink(bin->getElementByName("previewsink"));

    QGst::PadPtr videoPad = bin->getElementByName("voutqueue")->getStaticPad("src");
    bin->addPad(QGst::GhostPad::create(videoPad, "video"));

    QGst::PadPtr audioPad = bin->getElementByName("a2outqueue")->getStaticPad("src");
    bin->addPad(QGst::GhostPad::create(audioPad, "audio"));

    QGst::PadPtr audioPreListenPad = bin->getElementByName("a2prelistenqueue")->getStaticPad("src");
    bin->addPad(QGst::GhostPad::create(audioPreListenPad, "audioPreListen"));

    sourceOnline();

    return bin;
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

    /*
    // Record the stream to disk so it can be re-cut later
    icecastDumpProc = new QProcess(this);
    icecastDumpProc->start("gst-launch-1.0",
                           QStringList() << "souphttpsrc"
                                         << QString("location=%1%2").arg(iInfo.baseUrl.toString()).arg(mountName)
                                         << "!"
                                         << "filesink"
                                         << QString("location=%1/streaming/%2/aufnahmen/%3_%4")
                           .arg(QDir::homePath())
                           .arg(((MainWindow*) mainWin)->startUp.toString("yyyy-MM-dd_hh-mm-ss"))
                           .arg(QDateTime().currentDateTime().toString("yyyy-MM-dd_hh-mm-ss"))
                           .arg(mountName)
                           );*/
}

void CamBox::sourceOffline() {
    if (camOnline) return;

    qDebug() << "SOURCE ID" << name << "LEFT US";

    //ui->VideoPlayer->stop();
    //mediaSource->~MediaSource();
    //mediaSource = NULL;

    //KradClient::deleteStream(vPort.id);
    camOnline = false;
    updateBackground();

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
        if (ui->opacitySlider->value() != 0 || ui->volumeSlider->value() != 0) {
            // Source online and ONAIR (RED)
            p.setColor(QPalette::Window, Qt::red);
            this->setTitle(QString("%1 (ON AIR)").arg(name));
        } else {
            // Source online but not ONAIR (PALE GREEN)
            p.setColor(QPalette::Window, QColor(180, 255, 180));
            this->setTitle(QString("%1 (Bereit)").arg(name));
        }
    } else {
        // Source offline (LIGHT GRAY)
        p.setColor(QPalette::Window, Qt::lightGray);
        this->setTitle(QString("%1 (Nicht verbunden)").arg(name));
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
