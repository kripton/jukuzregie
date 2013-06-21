#include "cambox.h"
#include "ui_cambox.h"

CamBox::CamBox(QWidget *parent):
    QGroupBox(parent),
    ui(new Ui::CamBox)
{
    ui->setupUi(this);

    this->setAutoFillBackground(true);
    iInfo.sourceOnline = false;

    vPort.id = -1;

    this->mountName = mountName;

    // Initialize audio data viszualization
    dataOutput = new Phonon::AudioDataOutput(this);
    ui->AudioMeterSliderL->channel = Phonon::AudioDataOutput::LeftChannel;
    ui->AudioMeterSliderR->channel = Phonon::AudioDataOutput::RightChannel;
    connect(dataOutput, SIGNAL(dataReady(const QMap<Phonon::AudioDataOutput::Channel, QVector<qint16> >&)),
                                            ui->AudioMeterSliderL, SLOT(dataReceived(const QMap<Phonon::AudioDataOutput::Channel, QVector<qint16> >&)));
    connect(dataOutput, SIGNAL(dataReady(const QMap<Phonon::AudioDataOutput::Channel, QVector<qint16> >&)),
                                            ui->AudioMeterSliderR, SLOT(dataReceived(const QMap<Phonon::AudioDataOutput::Channel, QVector<qint16> >&)));

    // Set up timer to poll our icecast mount
    timer.setInterval(1000);
    connect(&timer, SIGNAL(timeout()), this, SLOT(pollIcecastRequest()));

    connect(ui->opacitySlider, SIGNAL(valueChanged(int)), this, SLOT(opcatiyFaderChanged()));
    connect(ui->volumeSlider, SIGNAL(valueChanged(int)), this, SLOT(volumeFaderChanged()));
    connect(ui->MonitorPushButton, SIGNAL(toggled(bool)), this, SLOT(MonitorPushButtonToggled(bool)));

    this->setAlignment(Qt::AlignHCenter);

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

void CamBox::setVideoOpacity(qreal opacity) {
    ui->opacitySlider->setValue(opacity*1000);
}

void CamBox::setKradVolume(qreal volume) {
    ui->volumeSlider->setValue(volume*10);
}

void CamBox::setPreListen(bool value)
{
    ui->MonitorPushButton->setChecked(value);
}

void CamBox::_setVideoOpacity(qreal opacity) {
    ui->opacitySlider->setValue(opacity*1000);
    if (vPort.opacity == opacity) return;
    vPort.opacity = opacity;

    if (ui->volSync->isChecked() && vPort.volume != opacity * 100.0) {
        _setKradVolume(opacity*100.0);
        return;
    }

    updateKradPort();
    updateBackGround();
}

void CamBox::_setKradVolume(qreal volume) {
    ui->volumeSlider->setValue(volume*10);
    if (vPort.volume == volume) return;
    vPort.volume = volume;
    updateKradPort();
    updateBackGround();
}

void CamBox::opcatiyFaderChanged()
{
    if (vPort.opacity == ui->opacitySlider->value() / 1000.0) return;
    _setVideoOpacity(ui->opacitySlider->value() / 1000.0);
}

void CamBox::volumeFaderChanged()
{
    if (vPort.volume == ui->volumeSlider->value() / 10.0) return;
    _setKradVolume(ui->volumeSlider->value() / 10.0);
}

void CamBox::setMountName(QString mountName)
{
    this->mountName = mountName;

    request.setUrl(iInfo.baseUrl);
    if (timer.isActive()) timer.stop();
    timer.start();
}

void CamBox::pollIcecastRequest()
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
}

void CamBox::sourceOnline() {
    if (iInfo.sourceOnline) return;

    mediaSource = new Phonon::MediaSource(QString("%1%2").arg(iInfo.baseUrl.toString()).arg(mountName));
    ui->VideoPlayer->play(*mediaSource);
    mute();

    ui->volumeSlider->setValue(0);
    ui->opacitySlider->setValue(0);
    ui->volumeSlider->setEnabled(true);
    ui->opacitySlider->setEnabled(true);

    vPort.pos_x = 0;
    vPort.pos_y = 0;
    vPort.width = 960;
    vPort.height = 540;
    vPort.crop_x = 0;
    vPort.crop_y = 0;
    vPort.crop_width = 960;
    vPort.crop_height = 540;
    vPort.opacity = 0.0f;
    vPort.rotation = 0.0f;
    vPort.volume = 0.0f;
    vPort.id = KradClient::playStream(QUrl(QString("%1%2").arg(iInfo.baseUrl.toString()).arg(mountName)));
    updateKradPort();

    // HACK: Set the opacity to 0 multiple time to not get it flicker on the screen
    QTimer::singleShot(80, this, SLOT(updateKradPort()));
    QTimer::singleShot(100, this, SLOT(updateKradPort()));
    QTimer::singleShot(120, this, SLOT(updateKradPort()));
    QTimer::singleShot(140, this, SLOT(updateKradPort()));
    QTimer::singleShot(160, this, SLOT(updateKradPort()));
    QTimer::singleShot(180, this, SLOT(updateKradPort()));

    qDebug() << "STREAM FROM" <<  QString("%1%2").arg(iInfo.baseUrl.toString()).arg(mountName) << "STARTED WITH KRAD ID:" << vPort.id;

    iInfo.sourceOnline = true;
    updateBackGround();
}

void CamBox::sourceOffline() {
    if (!iInfo.sourceOnline) return;

    qDebug() << "SOURCE ID" << vPort.id << "LEFT US";

    ui->VideoPlayer->stop();
    mediaSource->~MediaSource();
    mediaSource = NULL;

    KradClient::deleteStream(vPort.id);
    iInfo.sourceOnline = false;
    updateBackGround();

    ui->volumeSlider->setValue(0);
    ui->opacitySlider->setValue(0);
    ui->volumeSlider->setEnabled(false);
    ui->opacitySlider->setEnabled(false);
}

void CamBox::updateBackGround() {
    QPalette p = this->palette();
    if (iInfo.sourceOnline == true) {
        if (vPort.opacity != 0 || vPort.volume != 0) {
            // Source online and ONAIR (RED)
            p.setColor(QPalette::Window, Qt::red);
            this->setTitle(QString("%1 (ON AIR)").arg(mountName.toUpper()));
        } else {
            // Source online but on ONAIR (PALE GREEN)
            p.setColor(QPalette::Window, QColor(180, 255, 180));
            this->setTitle(QString("%1 (Bereit)").arg(mountName.toUpper()));
        }
    } else {
        // Source offline (LIGHT GRAY)
        p.setColor(QPalette::Window, Qt::lightGray);
        this->setTitle(QString("%1 (Nicht verbunden)").arg(mountName.toUpper()));
    }
    this->setPalette(p);
}

void CamBox::mute()
{
    ui->VideoPlayer->setVolume(0.0);
    qDebug() << "MUTED. Volume now:" << ui->VideoPlayer->volume();
}

void CamBox::unmute()
{
    ui->VideoPlayer->setVolume(1.0);
    qDebug() << "UNMUTED. Volume now:" << ui->VideoPlayer->volume();
}

void CamBox::MonitorPushButtonToggled(bool checked)
{
    if (checked) unmute(); else mute();
}

void CamBox::updateKradPort() {
    // Set KRAD's video port from vPort's values
    QStringList params;
    params << "setport"
           << QString("%1").arg(vPort.id + 1) // this is a bit odd. Link id is "0", video port is "1"
           << QString("%1").arg(vPort.pos_x)
           << QString("%1").arg(vPort.pos_y)
           << QString("%1").arg(vPort.width)
           << QString("%1").arg(vPort.height)
           << QString("%1").arg(vPort.crop_x)
           << QString("%1").arg(vPort.crop_y)
           << QString("%1").arg(vPort.crop_width)
           << QString("%1").arg(vPort.crop_height)
           << QString("%1").arg(vPort.opacity)
           << QString("%1").arg(vPort.rotation);
    KradClient::anyCommand(params);

    params.clear();
    params << "set" << QString("link%1").arg(vPort.id) << "volume" << QString("%1").arg(vPort.volume);
    KradClient::anyCommand(params);
}
