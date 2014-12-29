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
        connect(box, SIGNAL(fadeMeIn()), this, SLOT(fadeMeInHandler()));
        connect(box, SIGNAL(newOpacity(qreal)), this, SLOT(newOpacityHandler(qreal)));
        connect(box, SIGNAL(newVideoFrame(QImage)), this, SLOT(newVideoFrame(QImage)));
    }
    startUp = QDateTime::currentDateTime();
    QDir().mkpath(QString("%1/streaming/%2/aufnahmen/").arg(QDir::homePath()).arg(startUp.toString("yyyy-MM-dd_hh-mm-ss")));
    QDir().mkpath(QString("%1/streaming/%2/sprites/").arg(QDir::homePath()).arg(startUp.toString("yyyy-MM-dd_hh-mm-ss")));

    startupApplications();

    // Start the JACK-thread
    QThread* jackThread = new QThread;
    worker = new JackThread();
    worker->moveToThread(jackThread);

    connect(jackThread, SIGNAL(started()), worker, SLOT(setup()));
    connect(jackThread, SIGNAL(finished()), jackThread, SLOT(deleteLater()));

    connect(worker, SIGNAL(midiEvent(char, char, char)), this, SLOT(midiEvent(char, char, char)));

    connect(ui->recordButton, SIGNAL(toggled(bool)), this, SLOT(recordButtonToggled(bool)));
    connect(ui->transmitButton, SIGNAL(toggled(bool)), this, SLOT(transmitButtonToggled(bool)));
    connect(ui->textButton, SIGNAL(toggled(bool)), this, SLOT(textButtonToggled(bool)));
    connect(ui->logoButton, SIGNAL(toggled(bool)), this, SLOT(logoButtonToggled(bool)));

    rawvideocaps = QGst::Caps::fromString("video/x-raw,width=640,height=360,framerate=25/1");
    rawaudiocaps = QGst::Caps::fromString("audio/x-raw,format=F32LE,rate=48000,channels=2");

    QString audioPipeDesc = QString(" appsrc name=audiosource_main caps=\"%1\" is-live=true blocksize=8192 ! "
                                    " jackaudiosink provide-clock=false sync=false client-name=jukuzregie_main connect=0"
                                    " appsrc name=audiosource_monitor caps=\"%1\" is-live=true blocksize=8192 ! "
                                    " jackaudiosink provide-clock=false sync=false client-name=jukuzregie_monitor"
                                    " appsrc name=videosource is-live=true caps=\"%2\" ! videoconvert ! ximagesink")
            .arg("audio/x-raw,format=F32LE,rate=48000,layout=interleaved,channels=2")
            .arg("video/x-raw,format=BGRA,width=640,height=360,framerate=25/1,pixel-aspect-ratio=1/1");

    audioPipe = QGst::Parse::launch(audioPipeDesc).dynamicCast<QGst::Pipeline>();

    audioSrc_main = new AudioAppSrc(this);
    audioSrc_main->setElement(audioPipe->getElementByName("audiosource_main"));
    audioSrc_main->preAlloc = true;
    connect (audioSrc_main, SIGNAL(sigNeedData(uint, char*)), this, SLOT(prepareAudioData(uint, char*)));

    audioSrc_monitor = new AudioAppSrc(this);
    audioSrc_monitor->setElement(audioPipe->getElementByName("audiosource_monitor"));

    videoSrc = new VideoAppSrc(this);
    connect(videoSrc, SIGNAL(sigNeedData(uint)), this, SLOT(prepareVideoData(uint)));
    videoSrc->setElement(audioPipe->getElementByName("videosource"));

    QGlib::connect(audioPipe->bus(), "message", this, &MainWindow::onBusMessage);
    audioPipe->bus()->addSignalWatch();
    audioPipe->setState(QGst::StatePlaying);

    QImage backgroundSprite;
    backgroundSprite.load("/home/kripton/qtcreator/jukuzregie/sprites/pause-640x360.png");
    scene.addPixmap(QPixmap::fromImage(backgroundSprite));
    ui->videoBox->setScene(&scene);

    // Sockets for sending
    foreach (QHostAddress address, QNetworkInterface::allAddresses())
    {
        if (
                address != QHostAddress::Any &&
                address != QHostAddress::LocalHostIPv6 &&
                address != QHostAddress::AnyIPv6 &&
                address.toString().contains(".") &&
                !address.toString().contains(":")
        )
        {
            QUdpSocket* notifySocket = new QUdpSocket(this);
            notifySocket->bind(address, 12007);
            notifySockets.append(notifySocket);
        }
    }

    // Socket for incoming packets
    QUdpSocket* notifySocket = new QUdpSocket(this);
    notifySocket->bind(QHostAddress::Broadcast, 12007);
    connect(notifySocket, SIGNAL(readyRead()), this, SLOT(newNotifyDatagram()));

    QTimer* notificationTimer = new QTimer(this);
    notificationTimer->setInterval(1000);
    connect(notificationTimer, SIGNAL(timeout()), this, SLOT(broadcastSourceInfo()));
    notificationTimer->start();

    jackThread->start();
}

MainWindow::~MainWindow()
{
    if (!audioPipe.isNull())
    {
        audioPipe->setState(QGst::StateNull);
    }
    delete ui;
}

void MainWindow::processNotifyDatagram(QByteArray datagram, QHostAddress senderHost, quint16 senderPort)
{
    QDataStream stream(datagram);
    QHash<QString, QString> hash;
    stream >> hash;

    if ((hash.value("magic") != "JuKuZSourceCamConnect") || (senderPort != 4999))
    {
        return;
    }

    foreach (QObject* boxobj, allCamBoxes)
    {
        CamBox* box = (CamBox*) boxobj;
        if ((box->id == hash.value("id") || "" == hash.value("id")) && !box->getCamOnline())
        {
            if (hash.value("host") != "")
            {
                box->startCam(QHostAddress(hash.value("host")), hash.value("port").toUShort(), rawvideocaps, rawaudiocaps);
            }
            else
            {
                box->startCam(senderHost, hash.value("port").toUShort(), rawvideocaps, rawaudiocaps);
            }
            break;
        }
    }

    //foreach (QString key, hash.keys())
    //{
    //    qDebug() << key << hash.value(key);
    //}
}

void MainWindow::newNotifyDatagram()
{
    if (QObject::sender() == 0)
    {
        return;
    }

    QUdpSocket* notifySocket = ((QUdpSocket*)QObject::sender());
    while (notifySocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(notifySocket->pendingDatagramSize());
        QHostAddress senderHost;
        quint16 senderPort;

        notifySocket->readDatagram(datagram.data(), datagram.size(), &senderHost, &senderPort);

        //qDebug() << QString("DATAGRAM ON %1:%2 FROM %3:%4:")
        //            .arg(notifySocket->localAddress().toString())
        //            .arg(notifySocket->localPort())
        //            .arg(senderHost.toString())
        //            .arg(senderPort) << datagram;
        processNotifyDatagram(datagram, senderHost, senderPort);
    }
}

void MainWindow::broadcastSourceInfo()
{
    QHash<QString, QHash<QString, QString> > sources;
    foreach (QObject* boxobj, allCamBoxes)
    {
        CamBox* box = (CamBox*) boxobj;
        sources.insert(box->id, box->sourceInfo());
    }

    //qDebug() << "Broadcasting source info" << sources;

    QByteArray array;
    QDataStream stream(&array, QIODevice::WriteOnly);
    stream << sources;

    foreach (QUdpSocket* notifySocket, notifySockets)
    {
        notifySocket->writeDatagram(array, QHostAddress::Broadcast, 12007);
    }
}

void MainWindow::prepareAudioData(uint length, char* data)
{
    // Make sure the buffer has all float-zeroes
    #pragma omp parallel for
    for (uint i = 0; i < (length / sizeof(float)); i++)
    {
        ((float*)data)[i] = 0.0;
    }

    // Makes a deep copy, so the preListenData is also pre-filled with float-zeroes
    QByteArray preListenData = QByteArray(data, length);

    // Add the audio of all camBoxes
    foreach (QObject* obj, allCamBoxes)
    {
        CamBox* box = (CamBox*) obj;

        if (!box->getCamOnline())
        {
            continue;
        }

        qreal vol = box->getVolume();
        bool preListen = box->getPreListen();

        if ((vol == 0.0) && !preListen)
        {
            // Clear the currently queued audio data in the cambox
            // so we could use this to re-sync the audio data with the current input (video & from other camboxes)
            box->audioData.clear();
            continue;
        }

        if ((box->audioData.size() * sizeof(float)) < length)
        {
            qWarning() << "AUDIO BUFFER UNDERRUN! WANT" << length << "BYTES, CAMBOX" << box->id << "HAS" << box->audioData.size() * sizeof(float);
            // Don't dequeue anything from the box to give it a chance to catch up. This means that the buffer will not have any data from this camBox. THIS IS AUDIBLE!
            continue;
        }

        // No use of omp parallel for here since the .dequeue() needs to be done in the correct order!
        for (uint i = 0; i < (length / sizeof(float)); i++)
        {
            float sample = box->audioData.dequeue();
            ((float*)data)[i] += sample * vol;
            if (preListen)
            {
                ((float*)preListenData.data())[i] += sample;
            }

        }
    }

    // Push the buffer to the pipelines
    audioSrc_main->pushAudioBuffer();
    audioSrc_monitor->pushAudioBuffer(preListenData);
}

void MainWindow::prepareVideoData(uint length)
{
    Q_UNUSED(length)

    QImage vidImg(640, 360, QImage::Format_ARGB32);
    QPainter painter(&vidImg);
    //painter.setRenderHint(QPainter::Antialiasing);
    scene.render(&painter);

    QByteArray data((char*)vidImg.bits(), 640*360*4);

    videoSrc->pushVideoBuffer(data);
}

void MainWindow::onBusMessage(const QGst::MessagePtr & message)
{
    qDebug() << "MESSAGE" << message->type() << message->typeName();
    switch (message->type()) {
    case QGst::MessageError: //Some error occurred.
        qCritical() << message.staticCast<QGst::ErrorMessage>()->error() << message.staticCast<QGst::ErrorMessage>()->debugMessage();
        break;
    case QGst::MessageWarning:
        qWarning() << message.staticCast<QGst::WarningMessage>()->error() << message.staticCast<QGst::WarningMessage>()->debugMessage();
        break;
    case QGst::MessageStateChanged: //The element in message->source() has changed state
        qDebug() << "Pipeline NewState" << message.staticCast<QGst::StateChangedMessage>()->newState();
        break;
    default:
        break;
    }
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
        camBoxMgmtData* mgmtdata = new camBoxMgmtData;
        box->userData = mgmtdata;
        mgmtdata->pixmapItem = 0;
        mgmtdata->opacityEffect = 0;
        box->id = QString("cam_%1").arg(i, 2).replace(' ', '0');

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
      case 4: box = (CamBox*)ui->groupBox_5; break;
      case 5: box = (CamBox*)ui->groupBox_6; break;
      default: box = 0;
    }

    if (box == 0) return; // Button/Slider/Knob channel number is too high

    if (!box->getCamOnline()) return; // Source not online -> do nothing

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
        //fadeMeInHandler(box); TODO
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

void MainWindow::newOpacityHandler(qreal newValue)
{
    CamBox* sender = (CamBox*)QObject::sender();
    if (sender == 0) return;
    camBoxMgmtData* mgmtdata = (camBoxMgmtData*)sender->userData;
    if ((mgmtdata == 0) || (mgmtdata->opacityEffect == 0)) return;
    mgmtdata->opacityEffect->setOpacity(newValue);
}

void MainWindow::newVideoFrame(QImage image)
{
    // overlay it to the main scene
    CamBox* sender = (CamBox*)QObject::sender();
    if (sender == 0) return;
    camBoxMgmtData* mgmtdata = (camBoxMgmtData*)sender->userData;
    if (mgmtdata == 0) return;

    if (mgmtdata->pixmapItem == 0)
    {
        mgmtdata->pixmapItem = scene.addPixmap(QPixmap::fromImage(image));
        mgmtdata->opacityEffect = new QGraphicsOpacityEffect(this);
        mgmtdata->pixmapItem->setGraphicsEffect(mgmtdata->opacityEffect);
        mgmtdata->opacityEffect->setOpacity(0.0);
    }
    else
    {
        mgmtdata->pixmapItem->setPixmap(QPixmap::fromImage(image));
    }
}

void MainWindow::fadeMeInHandler()
{
    // Do 25 steps in one second => timer interval = 0.04s = 40ms
    foreach (QObject* boxObject, allCamBoxes) {
        CamBox* box = (CamBox*)boxObject;
        if (boxObject == QObject::sender())
        {
            box->fadeStart(0.04, 40);
        }
        else
        {
            box->fadeStart(-0.04, 40);
        }
    }
}

void MainWindow::setOnAirLED(QObject *boxObject, bool newState)
{
    CamBox* box = (CamBox*) boxObject;
    uchar num = box->id.split("_")[1].toUInt() - 1;

    worker->set_led(num, newState ? 0x7f : 0x00);
}
