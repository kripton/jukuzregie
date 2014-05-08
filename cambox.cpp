#include "cambox.h"
#include "ui_cambox.h"

CamBox::CamBox(QWidget *parent):
    QGroupBox(parent),
    ui(new Ui::CamBox)
{
    ui->setupUi(this);

    this->setAutoFillBackground(true);
    camOnline = false;

    // Initialize audio data viszualization
    //ui->VideoWidget->setVideoSink();

    // Set up timer to poll our icecast mount
    timer.setInterval(1000);
    connect(&timer, SIGNAL(timeout()), this, SLOT(pollIcecastRequest()));

    connect(ui->opacitySlider, SIGNAL(valueChanged(int)), this, SLOT(opcatiyFaderChanged()));
    connect(ui->volumeSlider, SIGNAL(valueChanged(int)), this, SLOT(volumeFaderChanged()));
    connect(ui->MonitorPushButton, SIGNAL(toggled(bool)), this, SLOT(MonitorPushButtonToggled(bool)));
    connect(ui->GOButton, SIGNAL(clicked()), this, SLOT(goButtonClicked()));

    this->setAlignment(Qt::AlignHCenter);

    fadeTimer = new QTimer(this);
    connect(fadeTimer, SIGNAL(timeout()), this, SLOT(fadeTimeEvent()));

    ui->volumeSlider->setValue(0);
    ui->opacitySlider->setValue(0);
    ui->volumeSlider->setEnabled(false);
    ui->opacitySlider->setEnabled(false);
    ui->MonitorPushButton->setEnabled(false);
    ui->GOButton->setEnabled(false);

    updateBackGround();
}

CamBox::~CamBox()
{
    delete ui;
}

bool CamBox::getPreListen()
{
    return ui->MonitorPushButton->isChecked();
}

void CamBox::setMainWindow(QObject *mainWin)
{
    this->mainWin = mainWin;
}

bool CamBox::isSourceOnline() {
    return camOnline;
}

QGst::Ui::VideoWidget *CamBox::VideoWidget()
{
    return ui->VideoWidget;
}

void CamBox::setVideoOpacity(qreal opacity) {
    ui->opacitySlider->setValue(opacity*1000);
}

void CamBox::setAudioVolume(qreal volume) {
    ui->volumeSlider->setValue(volume*10);
}

void CamBox::setPreListen(bool value)
{
    ui->MonitorPushButton->setChecked(value);
}

void CamBox::fadeStart(qint16 stepSize, qint16 interval)
{
    if (stepSize == 0) {
        fadeTimer->stop();
        return;
    }
    if (fadeTimer->isActive()) fadeTimer->stop();
    fadeStepSize = stepSize;
    fadeTimer->start(interval);
}

void CamBox::setName(QString name)
{
    this->name = name;

    if (timer.isActive()) timer.stop();
    timer.start();
}

/*void CamBox::pollIcecastRequest()
{
    qDebug() << mountName << "polling";
    reply = qnam.get(request);

    // execute an event loop to process the request (nearly-synchronous)
    QEventLoop eventLoop;
    // also dispose the event loop after the reply has arrived
    connect(&qnam, SIGNAL(finished(QNetworkReply *)), &eventLoop, SLOT(quit()));
    eventLoop.exec();

    if (reply->readAll().contains(QString("<td><h3>Mount Point /" + mountName + "</h3></td>").toAscii())) {
        qDebug() << "Online" << mountName;
        sourceOnline();
    } else {
        qDebug() << "Offline:" << mountName;
        sourceOffline();
    }
    updateBackGround();
}*/

void CamBox::sourceOnline() {
    if (camOnline) return;

    //mediaSource = new Phonon::MediaSource(QString("%1%2").arg(iInfo.baseUrl.toString()).arg(mountName));
    //ui->VideoPlayer->play(*mediaSource);
    mute();

    ui->volumeSlider->setValue(0);
    ui->opacitySlider->setValue(0);
    ui->volumeSlider->setEnabled(true);
    ui->opacitySlider->setEnabled(true);
    ui->MonitorPushButton->setEnabled(true);
    ui->GOButton->setEnabled(true);

    // HACK: Set the opacity to 0 multiple time to not get it flicker on the screen
    QTimer::singleShot(80, this, SLOT(updateKradPort()));
    QTimer::singleShot(100, this, SLOT(updateKradPort()));
    QTimer::singleShot(120, this, SLOT(updateKradPort()));
    QTimer::singleShot(140, this, SLOT(updateKradPort()));
    QTimer::singleShot(160, this, SLOT(updateKradPort()));
    QTimer::singleShot(180, this, SLOT(updateKradPort()));

    camOnline = true;
    updateBackGround();

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
    updateBackGround();

    ui->volumeSlider->setValue(0);
    ui->opacitySlider->setValue(0);
    ui->volumeSlider->setEnabled(false);
    ui->opacitySlider->setEnabled(false);
    ui->MonitorPushButton->setEnabled(false);
    ui->GOButton->setEnabled(false);
}

void CamBox::updateBackGround() {
    QPalette p = this->palette();
    /*if (camOnline == true) {
        if (vPort.opacity != 0 || vPort.volume != 0) {
            // Source online and ONAIR (RED)
            emit onAirInfo(this, true);
            p.setColor(QPalette::Window, Qt::red);
            this->setTitle(QString("%1 (ON AIR)").arg(mountName.toUpper()));
        } else {
            // Source online but not ONAIR (PALE GREEN)
            emit onAirInfo(this, false);
            p.setColor(QPalette::Window, QColor(180, 255, 180));
            this->setTitle(QString("%1 (Bereit)").arg(mountName.toUpper()));
        }
    } else {
        // Source offline (LIGHT GRAY)
        emit onAirInfo(this, false);
        p.setColor(QPalette::Window, Qt::lightGray);
        this->setTitle(QString("%1 (Nicht verbunden)").arg(mountName.toUpper()));
    }*/
    this->setPalette(p);
}

void CamBox::mute()
{
    //ui->VideoPlayer->setVolume(0.0);
    //qDebug() << "MUTED. Volume now:" << ui->VideoPlayer->volume();
}

void CamBox::unmute()
{
    //ui->VideoPlayer->setVolume(1.0);
    //qDebug() << "UNMUTED. Volume now:" << ui->VideoPlayer->volume();
}

void CamBox::MonitorPushButtonToggled(bool checked)
{
    if (checked) unmute(); else mute();
    emit preListenChanged(this, checked);
}

void CamBox::fadeTimeEvent()
{
    if ((fadeStepSize > 0) && ((ui->opacitySlider->value() + fadeStepSize) >= 1000)) {
        ui->opacitySlider->setValue(1000);
        fadeTimer->stop();
    } else if ((fadeStepSize < 0) && ((ui->opacitySlider->value() + fadeStepSize) <= 0)) {
        ui->opacitySlider->setValue(0);
        fadeTimer->stop();
    } else {
        ui->opacitySlider->setValue(ui->opacitySlider->value() + fadeStepSize);
    }
}

void CamBox::goButtonClicked()
{
    emit fadeMeIn(this);
}
