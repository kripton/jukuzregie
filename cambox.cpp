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
    connect(ui->MonitorPushButton, SIGNAL(toggled(bool)), this, SLOT(monitorButtonToggled(bool)));
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

void CamBox::setMainWindow(QObject *mainWin)
{
    this->mainWin = mainWin;
}

bool CamBox::isSourceOnline() {
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
    if (opacity == ui->opacitySlider->value() / 1000.0) return;
    emit newOpacity(this, ui->opacitySlider->value() / 1000.0);
}

void CamBox::volumeFaderChanged()
{
    if (volume == ui->volumeSlider->value() / 10.0) return;
    emit newVolume(this, ui->volumeSlider->value() / 10.0);
}

QGst::Ui::VideoWidget *CamBox::VideoWidget()
{
    return ui->VideoWidget;
}

void CamBox::setVideoOpacity(qreal opacity) {
    ui->opacitySlider->setValue(opacity*1000);
    // Does it emit a SIGNAL then? Should it?
}

void CamBox::setVolume(qreal volume) {
    ui->volumeSlider->setValue(volume*10);
    // Does it emit a SIGNAL then? Should it?
}

void CamBox::setPreListen(bool value)
{
    ui->MonitorPushButton->setChecked(value);
}

QGst::BinPtr CamBox::startCam(QHostAddress host, quint16 port, QGst::CapsPtr videocaps, QGst::CapsPtr audiocaps)
{
    qDebug() << "Starting stream from" << QString("%1:%2").arg(host.toString()).arg(port);
    QGst::BinPtr bin = QGst::Bin::create();
    QGst::ElementPtr videotestsrc = QGst::ElementFactory::make("videotestsrc");
    QGst::ElementPtr videotee = QGst::ElementFactory::make("tee");
    QGst::ElementPtr videoqueue = QGst::ElementFactory::make("queue");
    //QGst::ElementPtr videosinkqueue = QGst::ElementFactory::make("queue");
    //QGst::ElementPtr videosink = QGst::ElementFactory::make("qtvideosink");
    bin->add(videotestsrc, videotee, videoqueue);
    videotestsrc->link(videotee, videocaps);
    videotee->getRequestPad("src_%u")->link(videoqueue->getStaticPad("sink"));
    //videotee->getRequestPad("src_%u")->link(videosinkqueue->getStaticPad("sink"));
    //videosinkqueue->link(videosink);
    //VideoWidget()->setVideoSink(videosink);
    QGst::PadPtr videoPad = videoqueue->getStaticPad("src");
    bin->addPad(QGst::GhostPad::create(videoPad, "video"));
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

void CamBox::monitorButtonToggled(bool checked)
{
    emit newPreListen(this, checked);
}

void CamBox::goButtonClicked()
{
    emit fadeMeIn(this);
}
