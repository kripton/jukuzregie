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

    updateBackground();
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
    emit newOpacity(ui->opacitySlider->value() / 1000.0);
}

void CamBox::volumeFaderChanged()
{
    emit newVolume(ui->volumeSlider->value() / 10.0);
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

void CamBox::setPreListen(bool value)
{
    ui->MonitorPushButton->setChecked(value);
}

QGst::BinPtr CamBox::startCam(QHostAddress host, quint16 port, QGst::CapsPtr videocaps, QGst::CapsPtr audiocaps)
{
    qDebug() << "Starting stream from" << QString("%1:%2").arg(host.toString()).arg(port) << name;
    QString fileName = QString("/home/kripton/streaming/camvids/%1.ogg").arg(name);

    QString desc = QString("filesrc location=%2 ! decodebin ! videoscale ! %1 ! tee name=t ! queue ! xvimagesink name=previewsink t. ! queue name=voutqueue").arg(videocaps->toString()).arg(fileName);
    qDebug() << desc;
    QGst::BinPtr bin = QGst::Bin::fromDescription(desc, QGst::Bin::NoGhost);

    ui->VideoWidget->setVideoSink(bin->getElementByName("previewsink"));

    QGst::PadPtr videoPad = bin->getElementByName("voutqueue")->getStaticPad("src");
    bin->addPad(QGst::GhostPad::create(videoPad, "video"));

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

void CamBox::preListenButtonToggled(bool checked)
{
    emit newPreListen(checked);
}

void CamBox::goButtonClicked()
{
    emit fadeMeIn();
}
