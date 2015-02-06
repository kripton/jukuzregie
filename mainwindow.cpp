#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    startUp = QDateTime::currentDateTime();

    //////////////////// UI ////////////////////
    ui->setupUi(this);
    ui->textEdit->installEventFilter(this); // To catch Ctrl+Return on textEdit

    setWindowTitle(QString("%1 (instance id %2)").arg(windowTitle()).arg(startUp.toString("yyyy-MM-dd_hh-mm-ss")));

    logoLabel = new QLabel();
    logoLabel->setGeometry(0, 0, 640, 360);
    logoLabel->setAttribute(Qt::WA_TranslucentBackground, true );
    logoItem = scene.addWidget(logoLabel);
    logoItem->setGraphicsEffect(&logoOpacityEffect);
    logoItem->setZValue(1.0);
    logoOpacityEffect.setOpacity(0.0);

    textSpriteLabel = new QLabel();
    textSpriteLabel->setGeometry(0, 0, 640, 360);
    textSpriteLabel->setAttribute(Qt::WA_TranslucentBackground, true );
    textSpriteItem = scene.addWidget(textSpriteLabel);
    textSpriteItem->setGraphicsEffect(&textSpriteOpacityEffect);
    textSpriteItem->setZValue(0.9);
    textSpriteOpacityEffect.setOpacity(0.0);

    textFont.setPointSize(37);
    textFont.setBold(true);

    ui->textPosX->setValue(0.17);
    ui->textPosY->setValue(0.72); // 0.67 for two-lined texts
    connect(ui->textButton, SIGNAL(toggled(bool)), this, SLOT(textButtonToggled(bool)));
    connect(ui->logoButton, SIGNAL(toggled(bool)), this, SLOT(logoButtonToggled(bool)));
    connect(ui->logoFileSelectButton, SIGNAL(clicked()), this, SLOT(selectNewLogoFile()));

    connect(ui->textBackgroundSelectorBox, SIGNAL(clicked()), this, SLOT(selectNewTextBackground()));
    connect(ui->textFontConfigureButton, SIGNAL(clicked()), this, SLOT(editTextFont()));

    playButtonLEDState = false;
    playButtonBlinkTimer.setInterval(500);
    connect(&playButtonBlinkTimer, SIGNAL(timeout()), this, SLOT(playButtonBlinkTimerTimeout()));

    //////////////////// Paths for runtime dumping data and logging ////////////////////
    QString dumpDir = QString("%1/streaming/%2/aufnahmen/").arg(QDir::homePath()).arg(startUp.toString("yyyy-MM-dd_hh-mm-ss"));
    QDir().mkpath(dumpDir);

    if (!QDir().exists(QString("%1/streaming/%2/logs").arg(QDir::homePath()).arg(startUp.toString("yyyy-MM-dd_hh-mm-ss")))) {
        QDir().mkpath(QString("%1/streaming/%2/logs").arg(QDir::homePath()).arg(startUp.toString("yyyy-MM-dd_hh-mm-ss")));
    }

    //////////////////// MediaSources generics ////////////////////
    allSources << ui->groupBox_1 << ui->groupBox_2 << ui->groupBox_3 << ui->groupBox_4 << ui->groupBox_5 << ui->groupBox_6 << ui->groupBox_7 << ui->groupBox_8 << ui->groupBox_9 << ui->videoPlayer;

    foreach (MediaSourceBase* source, allSources)
    {
        camBoxMgmtData* mgmtdata = new camBoxMgmtData;
        source->userData = mgmtdata;

        mgmtdata->pixmapItem = scene.addPixmap(QPixmap());
        mgmtdata->opacityEffect = new QGraphicsOpacityEffect(this);
        mgmtdata->opacityEffect->setOpacity(0.0);
        mgmtdata->pixmapItem->setGraphicsEffect(mgmtdata->opacityEffect);

        connect(source, SIGNAL(fadeMeIn(bool)), this, SLOT(fadeMeInHandler(bool)));
        connect(source, SIGNAL(opacityChanged(qreal)), this, SLOT(newOpacityHandler(qreal)));
        connect(source, SIGNAL(volumeChanged(qreal)), this, SLOT(newVolumeHandler(qreal)));
        connect(source, SIGNAL(newVideoFrame(QImage)), this, SLOT(newVideoFrame(QImage)));
        connect(source, SIGNAL(preListenChanged(bool)), this, SLOT(newPrelistenHandler(bool)));
        connect(source, SIGNAL(pipelineNewState(QGst::State)), this, SLOT(stateChangedHandler(QGst::State)));
    }

    //////////////////// CamBoxes ////////////////////
    allCamBoxes << ui->groupBox_1 << ui->groupBox_2 << ui->groupBox_3 << ui->groupBox_4 << ui->groupBox_5 << ui->groupBox_6 << ui->groupBox_7 << ui->groupBox_8 << ui->groupBox_9;

    int i = 1;
    foreach (CamBox* box, allCamBoxes) {
        box->setDumpDir(dumpDir);
        box->setId(QString("cam_%1").arg(i, 2).replace(' ', '0'));
        i++;
    }

    camConnectDialog = new CamConnectDialog(allCamBoxes, this);
    camConnectDialog->hide();
    connect(ui->camConnectButton, SIGNAL(clicked()), camConnectDialog, SLOT(show()));
    connect(camConnectDialog, SIGNAL(connectCam(CamBox*,QHostAddress,quint16)), this, SLOT(startCam(CamBox*,QHostAddress,quint16)));

    videoAdjustmentDialog = new VideoAdjustmentDialog(allSources, this);
    videoAdjustmentDialog->hide();
    connect(ui->videoAdjustmentButton, SIGNAL(clicked()), videoAdjustmentDialog, SLOT(show()));

    videoEffectDialog = new VideoEffectDialog(allSources, this);
    videoEffectDialog->hide();
    connect(ui->videoEffectButton, SIGNAL(clicked()), videoEffectDialog, SLOT(show()));

    //////////////////// VideoPlayer ////////////////////
    camBoxMgmtData* mgmtdata = (camBoxMgmtData*)ui->videoPlayer->userData;
    mgmtdata->pixmapItem->setZValue(0.1); // draw the video on top of the camBoxes

    connect(ui->videoPlayer, SIGNAL(loopChanged(bool)), this, SLOT(loopChangedHandler(bool)));

    //////////////////// JACK thread for midi control ////////////////////
    QThread* jackThread = new QThread;
    worker = new JackThread();
    worker->moveToThread(jackThread);

    connect(jackThread, SIGNAL(started()), worker, SLOT(setup()));
    connect(jackThread, SIGNAL(finished()), jackThread, SLOT(deleteLater()));

    connect(worker, SIGNAL(midiEvent(char, char, char)), this, SLOT(handleMidiEvent(char, char, char)));

    //////////////////// UDP info broadcasting and listener for new source requests ////////////////////
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

    //////////////////// Video scene and preview box ////////////////////
    QImage backgroundSprite;
    backgroundSprite.load("/home/kripton/qtcreator/jukuzregie/sprites/pause-640x360.png");
    QGraphicsPixmapItem* item = scene.addPixmap(QPixmap::fromImage(backgroundSprite));
    item->setZValue(-0.1);
    ui->videoBox->setScene(&scene);

    //////////////////// GStreamer pipeline that handles the output and monitoring ////////////////////
    rawvideocaps = QString("video/x-raw,format=BGRA,width=640,height=360,framerate=25/1,pixel-aspect-ratio=1/1");
    rawaudiocaps = QString("audio/x-raw,format=F32LE,rate=48000,layout=interleaved,channels=2");

    ui->videoPlayer->init(rawvideocaps, rawaudiocaps);

    QString dumpFileName = QString("%1/out.webm").arg(dumpDir);

    // Basic parts: Audio MONITOR to JACK, Audio MAIN to JACK and to MUX to FILE, Video to MUX, MUX to tcpserversink
    QString outputPipeDesc = QString(" appsrc name=audiosrc_monitor caps=\"%1\" is-live=true blocksize=32768 ! "
                                     " audiorate ! jackaudiosink sync=false client-name=jukuzregie_monitor"

                                     " appsrc name=audiosrc_main caps=\"%1\" is-live=true blocksize=32768 format=time do-timestamp=true ! audiorate ! tee name=audio_main !"
                                     " queue ! jackaudiosink sync=false client-name=jukuzregie_main connect=0"
                                     " audio_main. ! queue ! audioconvert ! vorbisenc ! webmmux streamable=true name=mux ! tee name=muxout !"
                                     " queue ! filesink location=\"%4\" sync=false"

                                     " appsrc name=videosrc caps=\"%2\" is-live=true blocksize=%3 format=time do-timestamp=true ! videorate !"
                                     " videoconvert ! vp8enc threads=4 deadline=35000 ! mux."

                                     " muxout. ! queue ! tcpserversink host=0.0.0.0 port=6000") //sync-method=latest-keyframe
            .arg(rawaudiocaps)
            .arg(rawvideocaps)
            .arg(640*360*4)
            .arg(dumpFileName);

    outputPipe = QGst::Parse::launch(outputPipeDesc).dynamicCast<QGst::Pipeline>();

    audioSrc_main = new AudioAppSrc(this);
    audioSrc_main->setElement(outputPipe->getElementByName("audiosrc_main"));

    audioSrc_monitor = new AudioAppSrc(this);
    audioSrc_monitor->setElement(outputPipe->getElementByName("audiosrc_monitor"));
    audioSrc_monitor->preAlloc = true;
    connect (audioSrc_monitor, SIGNAL(sigNeedData(uint, char*)), this, SLOT(prepareAudioData(uint, char*)));

    videoSrc = new VideoAppSrc(this);
    connect(videoSrc, SIGNAL(sigNeedData(uint, char*)), this, SLOT(prepareVideoData(uint, char*)));
    videoSrc->setElement(outputPipe->getElementByName("videosrc"));

    QGlib::connect(outputPipe->bus(), "message", this, &MainWindow::onBusMessage);
    outputPipe->bus()->addSignalWatch();
    outputPipe->setState(QGst::StatePlaying);

    //////////////////// Start the midi control ////////////////////
    jackThread->start();

    for (unsigned char i = 0; i < 127; i++)
    {
        setLed(i, false);
    }
}

MainWindow::~MainWindow()
{
    if (!outputPipe.isNull())
    {
        outputPipe->setState(QGst::StateNull);
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
        if ((box->getId() == hash.value("id") || "" == hash.value("id")) && box->getState() == QGst::StateNull)
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
        sources.insert(box->getId(), box->getSourceInfo());
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

void MainWindow::startCam(CamBox *cam, QHostAddress host, quint16 port)
{
    if (cam->getState() != QGst::StateNull)
    {
        return;
    }
    cam->startCam(host, port, rawvideocaps, rawaudiocaps);
}

void MainWindow::prepareAudioData(uint length, char* data)
{
    // Make sure the buffer has all float-zeroes
    #pragma omp parallel for
    for (uint i = 0; i < (length / sizeof(float)); i++)
    {
        ((float*)data)[i] = 0.0;
    }

    // Makes a deep copy, so the mainData is also pre-filled with float-zeroes
    QByteArray mainAudioData = QByteArray(data, length);

    // Add the audio of all camBoxes and the videoPlayer
    foreach (MediaSourceBase* source, allSources)
    {
        if (source->getState() != QGst::StatePlaying)
        {
            continue;
        }

        qreal vol = source->getVolume();
        bool preListen = source->getMonitor();

        if ((vol == 0.0) && !preListen)
        {
            // Remove old samples but leave the latest ones in there, so we have one complete block remaining in the queue
            // This is done so we can re-sync the audio data with the current input (video & from other camboxes) by dropping old data in a time no one can hear it anyways
            source->trimQueuedSamples((int)length);
            continue;
        }

        if ((source->getQueuedSamplesCount() * sizeof(float)) < length)
        {
            // Don't dequeue anything from the box to give it a chance to catch up. This means that the buffer will not have any data from this camBox. THIS IS AUDIBLE!
            qWarning() << "AUDIO BUFFER UNDERRUN! WANT" << length << "BYTES, CAMBOX" << source->getId() << "HAS" << source->getQueuedSamplesCount() * sizeof(float);
            source->audioDiscontOn();
            continue;
        }

        // No use of omp parallel for here since the .dequeue() needs to be done in the correct order!
        for (uint i = 0; i < (length / sizeof(float)); i++)
        {
            float sample = source->dequeueSample();

            ((float*)mainAudioData.data())[i] += sample * vol;
            if (preListen)
            {
                ((float*)data)[i] += sample;
            }

        }
    }

    // Push the buffer to the pipelines
    audioSrc_monitor->pushAudioBuffer();
    audioSrc_main->pushAudioBuffer(mainAudioData);
}

void MainWindow::prepareVideoData(uint length, char* data)
{
    QImage vidImg(640, 360, QImage::Format_ARGB32);
    QPainter painter(&vidImg);
    //painter.setRenderHint(QPainter::Antialiasing);
    scene.render(&painter);

    memcpy((void*)data, (void*)vidImg.bits(), length);

    videoSrc->pushVideoBuffer();
}

bool MainWindow::eventFilter(QObject *obj, QEvent *ev)
{
    // Check for Ctrl+Enter and toggle textButton if so
    if ((obj == ui->textEdit) && (ev->type() == QEvent::KeyPress)) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(ev);

        if(keyEvent->key() == Qt::Key_Return && keyEvent->modifiers().testFlag(Qt::ControlModifier))
        {
            ui->textButton->toggle();
            return true;
        } else {
            // pass the event on to the parent class
            return QMainWindow::eventFilter(obj, ev);
        }
    }
    return QMainWindow::eventFilter(obj, ev);
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

void MainWindow::handleMidiEvent(char c0, char c1, char c2) {
    if ((uchar)c0 != 0xb0) return;
    qDebug() << "MIDI event:" << QString("Channel 0x%1 Value: 0x%2")
                .arg((short)c1,2,16, QChar('0'))
                .arg((short)c2,2,16, QChar('0'));

    float opacity = 0.0f;

    opacity = (float)c2 / (float)127;

    // Determine target by 2nd nibble
    MediaSourceBase* target = NULL;
    switch (c1 & 0x0f) {
      case 0: target = ui->groupBox_1; break;
      case 1: target = ui->groupBox_2; break;
      case 2: target = ui->groupBox_3; break;
      case 3: target = ui->groupBox_4; break;
      case 4: target = ui->groupBox_5; break;
      case 5: target = ui->groupBox_6; break;
      case 6: target = ui->groupBox_7; break;
      case 7: target = ui->groupBox_8; break;
      default: target = ui->videoPlayer; break;
    }

    if (target == NULL)
    {
        return; // Button/Slider/Knob channel number is too high for "normal" channel control
    }

    if (target == ui->videoPlayer)
    {
        if (c2 == 0) return; // no reaction on button up

        switch (c1)
        {
            case KNK2_Transport_Rev:
                ui->videoPlayer->setPosition(QTime(0, 0));
                break;

            case KNK2_Transport_Fwd:
                target->setPreListen(!target->getMonitor());
                break;

            case KNK2_Transport_Stop:
                ui->videoPlayer->stop();
                break;

            case KNK2_Transport_Play:
                ui->videoPlayer->playOrPause();
                break;

            case KNK2_Transport_Rec:
                fadeMeInHandler(false, target);
                break;

            case KNK2_Cycle:
                ui->videoPlayer->setLoop(!(ui->videoPlayer->getLoop()));
                break;

            case KNK2_Track_Left:
                ui->videoPlayer->prevFile();
                break;

            case KNK2_Track_Right:
                ui->videoPlayer->nextFile();
                break;
        }
    }
    else
    {
        if (target->getState() != QGst::StatePlaying)
        {
            return; // Source not online -> do nothing
        }

        // Determine action by 1st nibble
        switch (c1 & 0xf0)
        {
          case 0x00: // Fader = set opacity
            target->setVideoOpacity(opacity);
            return;

          case 0x10: // Knob
            return;

          case 0x20: // Solo = toggle prelisten
            if (c2 == 0) return; // no reaction on button up
            target->setPreListen(!target->getMonitor());
            return;

          case 0x30: // Mute
            return;

          case 0x40: // Rec = fade in this source, fade out all others
            if (c2 == 0) return; // no reaction on button up
            fadeMeInHandler(true, target);
            return;
        }
    }
}

void MainWindow::textButtonToggled(bool checked)
{
    QMovie* oldMovie = NULL;

    if (checked) {
        textItem = scene.addText(ui->textEdit->toPlainText(), textFont);
        textItem->setPos(ui->textPosX->value()*640, ui->textPosY->value()*360);
        textItem->setZValue(0.95);
        scene.setSceneRect(0, 0, 640, 360);

        QFileInfo fInfo(ui->textSpriteFilename->text());

        if (!fInfo.exists() || !fInfo.isFile())
        {
            return;
        }

        QMovie *movie = new QMovie(ui->textSpriteFilename->text());

        if (!movie->isValid())
        {
            return;
        }

        oldMovie = textSpriteLabel->movie();

        movie->setScaledSize(QSize(640,360));
        movie->setCacheMode(QMovie::CacheAll);
        textSpriteLabel->setMovie(movie);
        movie->start();

        if (oldMovie != NULL)
        {
            oldMovie->stop();
            free(oldMovie);
        }

        textSpriteOpacityEffect.setOpacity(1.0);
    } else {
        scene.removeItem((QGraphicsItem*)textItem);
        textSpriteOpacityEffect.setOpacity(0.0);
    }
}

void MainWindow::logoButtonToggled(bool checked)
{
    QMovie* oldMovie = NULL;

    if (checked) {
        QFileInfo fInfo(ui->logoFileLineEdit->text());

        if (!fInfo.exists() || !fInfo.isFile())
        {
            return;
        }

        QMovie *movie = new QMovie(ui->logoFileLineEdit->text());

        if (!movie->isValid())
        {
            return;
        }

        oldMovie = logoLabel->movie();

        movie->setScaledSize(QSize(640,360));
        movie->setCacheMode(QMovie::CacheAll);
        logoLabel->setMovie(movie);
        movie->start();

        if (oldMovie != NULL)
        {
            oldMovie->stop();
            free(oldMovie);
        }

        logoOpacityEffect.setOpacity(1.0);
    } else {
        logoOpacityEffect.setOpacity(0.0);
    }
}

void MainWindow::selectNewLogoFile()
{
    ui->logoFileLineEdit->setText(QFileDialog::getOpenFileName(this, tr("Open Logo"), "", tr("PNG or MNG images (*.png *.mng)")));
}

void MainWindow::editTextFont()
{
    textFont = QFontDialog::getFont(0, textFont);
}

void MainWindow::selectNewTextBackground()
{
    ui->textSpriteFilename->setText(QFileDialog::getOpenFileName(this, tr("Open Textsprite"), "", tr("PNG or MNG images (*.png *.mng)")));
}

void MainWindow::newOpacityHandler(qreal newValue)
{
    MediaSourceBase* sender = (MediaSourceBase*)QObject::sender();
    if (sender == 0) return;
    camBoxMgmtData* mgmtdata = (camBoxMgmtData*)sender->userData;
    if ((mgmtdata == 0) || (mgmtdata->opacityEffect == 0)) return;

    mgmtdata->opacityEffect->setOpacity(newValue);

    if ((newValue > 0.0) || (sender->getVolume() > 0.0))
    {
        setOnAirLED(sender, true);
    }
    else
    {
        setOnAirLED(sender, false);
    }
}

void MainWindow::newVolumeHandler(qreal newValue)
{
    MediaSourceBase* sender = (MediaSourceBase*)QObject::sender();
    if (sender == 0) return;
    camBoxMgmtData* mgmtdata = (camBoxMgmtData*)sender->userData;
    if ((mgmtdata == 0) || (mgmtdata->opacityEffect == 0)) return;

    if ((newValue > 0.0) || (mgmtdata->opacityEffect->opacity() > 0.0))
    {
        setOnAirLED(sender, true);
    }
    else
    {
        setOnAirLED(sender, false);
    }
}

void MainWindow::newPrelistenHandler(bool newState)
{
    MediaSourceBase* sender = (MediaSourceBase*)QObject::sender();
    if (sender == 0) return;

    if (sender->getId().startsWith("cam_"))
    {
        uchar num = sender->getId().split("_")[1].toUInt() - 1;
        setLed(num + 0x20, newState);
    }
    else if (sender->getId() == "VideoPlayer")
    {
        setLed(KNK2_Transport_Fwd, newState);
    }
}

void MainWindow::newVideoFrame(QImage image)
{
    // overlay it to the main scene
    MediaSourceBase* sender = (MediaSourceBase*)QObject::sender();
    if (sender == 0) return;
    camBoxMgmtData* mgmtdata = (camBoxMgmtData*)sender->userData;
    if (mgmtdata == 0) return;

    mgmtdata->pixmapItem->setPixmap(QPixmap::fromImage(image));
}

void MainWindow::loopChangedHandler(bool newState)
{
    setLed(KNK2_Cycle, newState);
}

void MainWindow::stateChangedHandler(QGst::State newState)
{
    MediaSourceBase* sender = (MediaSourceBase*)QObject::sender();
    if (sender == 0) return;

    bool state = false;
    if (newState == QGst::StatePlaying)
    {
        state = true;
    }

    if (sender->getId().startsWith("cam_"))
    {
        uchar num = sender->getId().split("_")[1].toUInt() - 1;
        setLed(num + 0x30, state);
    }
    else if (sender->getId() == "VideoPlayer")
    {
        if (state)
        {
            // Now Playing, set the LED to ON and stop the pause blink timer
            playButtonBlinkTimer.stop();
            setLed(KNK2_Transport_Play, true);
            playButtonLEDState = true;
        }
        else
        {
            playButtonBlinkTimer.start();
        }
    }
}

void MainWindow::playButtonBlinkTimerTimeout()
{
    setLed(KNK2_Transport_Play, !playButtonLEDState);
    playButtonLEDState = !playButtonLEDState;
}

void MainWindow::fadeMeInHandler(bool fadeOutOthers, MediaSourceBase* sourceOverride)
{
    // Do 25 steps in one second => timer interval = 0.04s = 40ms
    foreach (MediaSourceBase* source, allSources) {
        if ((source == QObject::sender()) || (source == sourceOverride))
        {
            source->fadeStart(0.04, 40);
        }
        else if (fadeOutOthers)
        {
            source->fadeStart(-0.04, 40);
        }
    }
}

void MainWindow::setOnAirLED(MediaSourceBase *boxObject, bool newState)
{
    if (boxObject->getId().startsWith("cam_"))
    {
        uchar num = boxObject->getId().split("_")[1].toUInt() - 1;
        setLed(num + KNK2_Rec1, newState);
    }
    else if (boxObject->getId() == "VideoPlayer")
    {
        setLed(KNK2_Transport_Rec, newState);
    }
}

void MainWindow::setLed(unsigned char num, bool newState)
{
    worker->set_led(num, newState ? 0x7f : 0x00);
}
