#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    allCamBoxes << ui->groupBox << ui->groupBox_2 << ui->groupBox_3 << ui->groupBox_4 << ui->groupBox_5 << ui->groupBox_6;

    foreach (QObject* boxObject, allCamBoxes) {
        CamBox* box = (CamBox*)boxObject;
        box->setMainWindow(this);
        connect(box, SIGNAL(newOpacity(QObject*,qreal)), this, SLOT(cb_newOpacity(QObject*, qreal)));
        connect(box, SIGNAL(fadeMeIn(QObject*)), this, SLOT(fadeInOneFadeOutOther(QObject*)));
        connect(box, SIGNAL(preListenChanged(QObject*,bool)), this, SLOT(preListenChangedHandler(QObject*,bool)));
        connect(box, SIGNAL(onAirInfo(QObject*,bool)), this, SLOT(onAirInfoHandler(QObject*,bool)));
    }
    startUp = QDateTime::currentDateTime();
    QDir().mkpath(QString("%1/streaming/%2/aufnahmen/").arg(QDir::homePath()).arg(startUp.toString("yyyy-MM-dd_hh-mm-ss")));
    QDir().mkpath(QString("%1/streaming/%2/sprites/").arg(QDir::homePath()).arg(startUp.toString("yyyy-MM-dd_hh-mm-ss")));

    startupApplications();

    // Start the JACK-thread
    QThread* thread = new QThread;
    worker = new JackThread();
    worker->moveToThread(thread);

    connect(thread, SIGNAL(started()), worker, SLOT(setup()));
    connect(worker, SIGNAL(finished()), thread, SLOT(quit()));
    connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

    connect(worker, SIGNAL(midiEvent(char, char, char)), this, SLOT(midiEvent(char, char, char)));

    connect(ui->recordButton, SIGNAL(toggled(bool)), this, SLOT(recordButtonToggled(bool)));
    connect(ui->transmitButton, SIGNAL(toggled(bool)), this, SLOT(transmitButtonToggled(bool)));
    connect(ui->textButton, SIGNAL(toggled(bool)), this, SLOT(textButtonToggled(bool)));
    connect(ui->logoButton, SIGNAL(toggled(bool)), this, SLOT(logoButtonToggled(bool)));

    rawvideocaps = QGst::Caps::fromString("video/x-raw,width=640,height=360,framerate=25/1");

    Pipeline = QGst::Pipeline::create();

    // Background image "bin"
    QGst::ElementPtr filesrc = QGst::ElementFactory::make("filesrc");
    filesrc->setProperty("location", "/home/kripton/qtcreator/jukuzregie/sprites/pause-640x360.png");
    QGst::ElementPtr pngdec = QGst::ElementFactory::make("pngdec");
    QGst::ElementPtr videoconvert = QGst::ElementFactory::make("videoconvert");
    QGst::ElementPtr imagefreeze = QGst::ElementFactory::make("imagefreeze");
    QGst::ElementPtr queue = QGst::ElementFactory::make("queue");

    QGst::BinPtr backgroundBin = QGst::Bin::create();
    backgroundBin->add(filesrc, pngdec, videoconvert, imagefreeze, queue);

    filesrc->link(pngdec);
    pngdec->link(videoconvert);
    videoconvert->link(imagefreeze);
    imagefreeze->link(queue, rawvideocaps);
    QGst::PadPtr sourcePad = queue->getStaticPad("src");
    backgroundBin->addPad(QGst::GhostPad::create(sourcePad, "videoSource"));

    VideoMixer = QGst::ElementFactory::make("videomixer");
    VideoMixerTee = QGst::ElementFactory::make("tee");

    VideoSinkPreview = QGst::ElementFactory::make("qtvideosink");

    Pipeline->add(backgroundBin, VideoMixer, VideoMixerTee, VideoSinkPreview);

    backgroundBin->getStaticPad("videoSource")->link(VideoMixer->getRequestPad("sink_%u"));

    VideoMixer->link(VideoMixerTee);
    VideoMixerTee->link(VideoSinkPreview);

    ui->VideoPlayer->setVideoSink(VideoSinkPreview);

    Pipeline->setState(QGst::StatePlaying);

    notifySocket = new QUdpSocket(this);
    notifySocket->bind(12007);
    connect(notifySocket, SIGNAL(readyRead()), this, SLOT(newNotifyDatagram()));

    QTimer* notificationTimer = new QTimer(this);
    notificationTimer->setInterval(1000);
    connect(notificationTimer, SIGNAL(timeout()), this, SLOT(broadcastSourceInfo()));
    //notificationTimer->start();

    thread->start();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::processNotifyDatagram(QByteArray datagram, QHostAddress senderHost, quint16 senderPort)
{
    QDataStream* stream = new QDataStream(datagram);
    QHash<QString, QString>* hash = new QHash<QString, QString>();
    (*stream) >> (*hash);

    if (hash->value("magic") != "JuKuZSourceCamConnect") return;

    foreach (QObject* boxobj, allCamBoxes)
    {
        CamBox* box = (CamBox*) boxobj;
        if ((box->name == hash->value("name")) && !box->isSourceOnline())
        {
            QGst::BinPtr bin = box->startCam(senderHost, senderPort + 1, rawvideocaps, rawaudiocaps);
            Pipeline->add(bin);
            QGst::PadPtr sinkPad = VideoMixer->getRequestPad("sink_%u");
            boxMixerPads.insert(boxobj, sinkPad);
            bin->getStaticPad("video")->link(sinkPad);
            bin->syncStateWithParent();
        }
    }

    foreach (QString key, hash->keys())
    {
        qDebug() << key << hash->value(key);
    }
}

void MainWindow::newNotifyDatagram()
{
    while (notifySocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(notifySocket->pendingDatagramSize());
        QHostAddress senderHost;
        quint16 senderPort;

        notifySocket->readDatagram(datagram.data(), datagram.size(), &senderHost, &senderPort);

        qDebug() << "FROM" << senderHost << senderPort << datagram;
        processNotifyDatagram(datagram, senderHost, senderPort);
    }
}

void MainWindow::broadcastSourceInfo()
{
    QHash<QString, QHash<QString, QString> >* sources = new QHash<QString, QHash<QString, QString> >();
    foreach (QObject* boxobj, allCamBoxes)
    {
        CamBox* box = (CamBox*) boxobj;
        sources->insert(box->name, box->sourceInfo());
    }
    QByteArray* array = new QByteArray();
    QDataStream* stream = new QDataStream(array, QIODevice::WriteOnly);
    (*stream) << *sources;
    notifySocket->writeDatagram(*array, QHostAddress::Broadcast, 12007);
}

void MainWindow::cb_newOpacity(QObject *sender, qreal newValue)
{
    qDebug() << "NewOpacity" << newValue << "from" << sender << boxMixerPads[sender]->name();
    boxMixerPads[sender]->setProperty("alpha", newValue);
}

void MainWindow::startupApplications() {
    if (!QDir().exists(QString("%1/streaming/%2/logs").arg(QDir::homePath()).arg(startUp.toString("yyyy-MM-dd_hh-mm-ss")))) {
        QDir().mkpath(QString("%1/streaming/%2/logs").arg(QDir::homePath()).arg(startUp.toString("yyyy-MM-dd_hh-mm-ss")));
    }
}

void MainWindow::start() {
    int i = 1;
    foreach (QObject* boxObject, allCamBoxes) {
        CamBox* box = (CamBox*) boxObject;

        box->name = QString("cam_%1").arg(i, 2).replace(' ', '0');
        i++;
    }
}

void MainWindow::midiEvent(char c0, char c1, char c2) {
    if ((uchar)c0 != 0xb0) return;
    qDebug() << "MIDI event:" << QString("Channel 0x%1 Value: 0x%2")
                .arg((short)c1,2,16, QChar('0'))
                .arg((short)c2,2,16, QChar('0'));

    float opacity = 0.0f;

    opacity = (float)c2 / (float)127;


    // Determine target by 2nd nibble
    CamBox* box;
    switch (c1 & 0x0f) {
      case 0: box = (CamBox*)ui->groupBox; break;
      case 1: box = (CamBox*)ui->groupBox_2; break;
      case 2: box = (CamBox*)ui->groupBox_3; break;
      case 3: box = (CamBox*)ui->groupBox_4; break;
    }

    if (box == NULL) return; // Button/Slider/Knob channel number is too high

    if (!box->isSourceOnline()) return; // Source not online -> do nothing

    // Determine action by 1st nibble
    switch (c1 & 0xf0) {
      case 0x00: // Fader = set opacity
        box->setVideoOpacity(opacity);
        return;

      case 0x10: // Knob
        return;

      case 0x20: // Solo = toggle prelisten
        if (c2 == 0) return; // no reaction on button up
        box->setPreListen(!box->getPreListen());
        return;

      case 0x30: // Mute
        return;

      case 0x40: // Rec = fade in this source, fade out all others
        if (c2 == 0) return; // no reaction on button up
        fadeInOneFadeOutOther(box);
        return;

    }
}

void MainWindow::recordButtonToggled(bool checked)
{
    if (checked) {
        if (!QDir().exists(QString("%1/streaming/%2/aufnahmen").arg(QDir::homePath()).arg(startUp.toString("yyyy-MM-dd_hh-mm-ss")))) {
            QDir().mkpath(QString("%1/streaming/%2/aufnahmen").arg(QDir::homePath()).arg(startUp.toString("yyyy-MM-dd_hh-mm-ss")));
        }
    } else {
    }
}

void MainWindow::transmitButtonToggled(bool checked)
{
    if (checked) {
    } else {
    }
}

void MainWindow::textButtonToggled(bool checked)
{
    if (checked) {
        ui->textButton->setText("Text deaktivieren");

        // Render the text to image using imagemagick's convert
        QString fileName = QString("%1/streaming/%2/sprites/%3.png")
                .arg(QDir::homePath())
                .arg(startUp.toString("yyyy-MM-dd_hh-mm-ss"))
                .arg(QDateTime().currentDateTime().toString("yyyy-MM-dd_hh-mm-ss"));
        QProcess* imProc = new QProcess();
        imProc->start("convert", QStringList() <<
                     "-size" << "960x540" << "canvas:transparent" <<
                     "-font" << "Impact" << "-pointsize" << "48" <<
                     "-fill" << "black" <<
                      "-draw" << QString("text 180,440 '%1'").arg(ui->textEdit->text().replace("'", "\\'")) <<
                     fileName);
        imProc->waitForFinished();

    } else {
        ui->textButton->setText("Text aktivieren");
    }
}

void MainWindow::logoButtonToggled(bool checked)
{
    if (checked) {
    } else {
    }
}

void MainWindow::fadeInOneFadeOutOther(QObject *fadeInBox)
{
    // TIMER!
    foreach (QObject* boxObject, allCamBoxes) {
        CamBox* box = (CamBox*) boxObject;
        if (box == fadeInBox) {

        } else {

        }
    }
}

void MainWindow::preListenChangedHandler(QObject *sender, bool newState)
{
    int i = 0;
    foreach (QObject* boxObject, allCamBoxes) {
        CamBox* box = (CamBox*) boxObject;
        if (box == sender) {
            break;
        }
        i++;
    }
    worker->set_led(i + 0x20, newState ? 0x7f : 0x00);
}

void MainWindow::onAirInfoHandler(QObject *sender, bool newState)
{
    int i = 0;
    foreach (QObject* boxObject, allCamBoxes) {
        CamBox* box = (CamBox*) boxObject;
        if (box == sender) {
            break;
        }
        i++;
    }
    worker->set_led(i + 0x40, newState ? 0x7f : 0x00);
}
