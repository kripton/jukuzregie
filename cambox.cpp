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

void CamBox::startCam(QHostAddress host, quint16 port, QGst::CapsPtr videocaps, QGst::CapsPtr audiocaps)
{
    qDebug() << "Starting stream from" << QString("%1:%2").arg(host.toString()).arg(port) << name;
    QString dumpFileName = QString("/home/kripton/streaming/dump/%1.mkv").arg(name);


    QString desc = QString("tcpclientsrc host=%1 port=%2 ! gdpdepay ! tee name=stream ! decodebin name=decode ! "
                           "queue ! videoscale ! %3 ! tee name=video ! queue ! xvimagesink name=previewsink"
                           "video. ! videoconvert ! appsink name=\"mysink\" caps=\"%2\""
                           "stream. ! queue ! mkvmux ! filesink location=\"%6\"")
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

    qDebug() << "SOURCE ID" << name << "LEFT US";

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
