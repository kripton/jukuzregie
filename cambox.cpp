#include "cambox.h"
#include "ui_cambox.h"

CamBox::CamBox(QWidget *parent):
    MediaSourceBase(parent),
    ui(new Ui::CamBox)
{
    ui->setupUi(this);
    MediaSourceBase::init(ui->AudioMeterSliderL, ui->AudioMeterSliderR, ui->audioLabel, ui->GOButton, ui->VideoBox, ui->opacitySlider, ui->fadeCheck, ui->MonitorPushButton, ui->volumeSlider, ui->volSync);

    connect(ui->disconnectButton, SIGNAL(clicked()), this, SLOT(disconnectSource()));

    ui->disconnectButton->setEnabled(false);

    // This is a bit awkward. Somehow, the title gets changed back
    // if we just call the SLOT here. So, use a single-shot timer
    QTimer::singleShot(200, this, SLOT(updateTitle()));
}

CamBox::~CamBox()
{
    delete ui;
}

void CamBox::setDumpDir(QString dir)
{
    dumpDir = dir;
}

void CamBox::disconnectSource()
{
    setVideoOpacity(0.0);
    setVolume(0.0);
    if (!pipeline.isNull())
    {
        pipeline->setState(QGst::StateNull);
        pipeline.clear();
    }
    sourceOffline();
}

void CamBox::onBusMessage(const QGst::MessagePtr &message)
{
    //qDebug() << "CamBox::MESSAGE" << message->type() << message->typeName();
    switch (message->type()) {
        case QGst::MessageStateChanged:
            updateTitle();
            break;
        case QGst::MessageTag:
        {
            QGst::TagList taglist = message.staticCast<QGst::TagMessage>()->taglist();
            if (taglist.comment() != "")
            {
                name = taglist.comment();
                updateTitle();
            }
            break;
        }

        default:
            break;
    }
}

void CamBox::startCam(QHostAddress host, quint16 port, QString videocaps, QString audiocaps)
{
    qDebug() << "Starting stream from" << QString("%1:%2").arg(host.toString()).arg(port) << id;
    QString dumpFileName = QString("%1/%2_%3.mkv")
            .arg(dumpDir)
            .arg(QDateTime::currentDateTime().toString("yyyy-mm-dd_hh-mm-ss"))
            .arg(id);

    QString desc = QString("appsrc name=source !"
                   " decodebin name=decode ! queue ! videoscale ! videoconvert ! videorate ! "
                   " gamma name=gamma ! videobalance name=balance ! videoflip name=flip ! "
                   " appsink name=videosink caps=\"%1\""
                   " decode. ! queue ! audioconvert ! audioresample ! appsink name=audiosink caps=\"%2\"")
            .arg(videocaps)
            .arg(audiocaps);

    qDebug() << "Pipeline:" << desc;
    pipeline = QGst::Parse::launch(desc).dynamicCast<QGst::Pipeline>();

    QGlib::connect(pipeline->bus(), "message", (MediaSourceBase*)this, &MediaSourceBase::onBusMessage);
    QGlib::connect(pipeline->bus(), "message", this, &CamBox::onBusMessage);
    pipeline->bus()->addSignalWatch();

    m_tcpsrc.setElement(pipeline->getElementByName("source"));
    m_tcpsrc.start(host.toString(), port, dumpFileName);

    videoSink.setElement(pipeline->getElementByName("videosink"));

    audioSink.setElement(pipeline->getElementByName("audiosink"));

    gammaElement = pipeline->getElementByName("gamma");
    videoBalanceElement = pipeline->getElementByName("balance");
    videoFlipElement = pipeline->getElementByName("flip");

    // start playing
    pipeline->setState(QGst::StatePlaying);

    sourceOnline();
}


void CamBox::sourceOnline() {
    MediaSourceBase::sourceOnline();

    ui->disconnectButton->setEnabled(true);
}

void CamBox::sourceOffline() {
    MediaSourceBase::sourceOffline();

    m_tcpsrc.stop();

    ui->disconnectButton->setEnabled(false);
}

void CamBox::updateTitle() {
    switch (getState())
    {
        case QGst::StateReady:
        case QGst::StatePaused:
            // Source online but pipeline not PLAYING ... (Still buffering input)
            setTitle(QString("%1 (%2) : Buffering").arg(id).arg(name));
            break;
        case QGst::StatePlaying:
            if ((ui->opacitySlider->value() != 0) || (ui->volumeSlider->value() != 0)) {
                // Source online and ONAIR (RED)
                setTitle(QString("%1 (%2) : ON AIR").arg(id).arg(name));
            } else {
                // Source online but not ONAIR (PALE GREEN)
                setTitle(QString("%1 (%2) : Bereit").arg(id).arg(name));
            }
            break;
        case QGst::StateNull:
           // Source offline (LIGHT GRAY)
           setTitle(QString("%1 : Nicht verbunden").arg(id));
            break;

        default:
            break;
    }
}
