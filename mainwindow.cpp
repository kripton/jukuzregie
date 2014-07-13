#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    logoSpriteId = -1;
    textBgSpriteId = -1;
    textSpriteId = -1;

    allCamBoxes << ui->groupBox_1 << ui->groupBox_2 << ui->groupBox_3 << ui->groupBox_4
                << ui->groupBox_5 << ui->groupBox_6 << ui->groupBox_7 << ui->groupBox_8 << ui->groupBox_9;

    foreach (QObject* boxObject, allCamBoxes) {
        CamBox* box = (CamBox*)boxObject;
        box->setMainWindow(this);
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

    thread->start();
}

MainWindow::~MainWindow()
{
    KradClient::kill();
    westonprocess->kill();
    delete ui;
}

void MainWindow::startupApplications() {
    if (!QDir().exists(QString("%1/streaming/xdg").arg(QDir::homePath()))) {
        QDir().mkpath(QString("%1/streaming/xdg").arg(QDir::homePath()));
    }
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("XDG_RUNTIME_DIR", QString("%1/streaming/xdg").arg(QDir::homePath()));

    if (!QSettings().value("general/noWeston", false).toBool()) {
        qDebug() << "Starting up weston ...";
        westonprocess = new QProcess();
        westonprocess->setProcessEnvironment(env);
        westonprocess->start("weston", QStringList() << "--idle-time=0");
        westonprocess->waitForStarted();
        qDebug() << "Weston has been started";
    }

    qDebug() << "Starting up KRAD";
    KradClient::launch();

    if (!QDir().exists(QString("%1/streaming/%2/logs").arg(QDir::homePath()).arg(startUp.toString("yyyy-MM-dd_hh-mm-ss")))) {
        QDir().mkpath(QString("%1/streaming/%2/logs").arg(QDir::homePath()).arg(startUp.toString("yyyy-MM-dd_hh-mm-ss")));
    }
    KradClient::anyCommand(QStringList() << "setdir" << QString("%1/streaming/%2/logs/").arg(QDir::homePath()).arg(startUp.toString("yyyy-MM-dd_hh-mm-ss")));
    KradClient::anyCommand(QStringList() << "res" << "960" << "540");
    KradClient::anyCommand(QStringList() << "setres" << "960" << "540");
    KradClient::anyCommand(QStringList() << "fps" << "25");
    KradClient::anyCommand(QStringList() << "setfps" << "25");
    KradClient::anyCommand(QStringList() << "rate" << "48000");
    KradClient::anyCommand(QStringList() << "setrate" << "48000");
    if (!QSettings().value("general/noWeston", false).toBool()) {
        KradClient::anyCommand(QStringList() << "display");
    }
    KradClient::anyCommand(QStringList() << "output" << "jack");
    qDebug() << "KRAD has been started";
}

void MainWindow::start() {
    QUrl baseUrl = QUrl(
                QString("http://%1:%2/")
                .arg(QSettings().value("inbound/host", "127.0.0.1").toString())
                .arg(QSettings().value("inbound/port", "12000").toString())
                );
    qDebug() << "Inbound baseUrl" << baseUrl;

    int i = 1;
    foreach (QObject* boxObject, allCamBoxes) {
        CamBox* box = (CamBox*) boxObject;
        box->iInfo.baseUrl = baseUrl;
        box->setMountName(QString("cam_%1.%2")
                          .arg(i, 2).replace(' ', '0')
                          .arg(QSettings().value("inbound/fileExt", "12000").toString())
                          );
        qDebug() << "MountName:" << QString("cam_%1.%2")
                    .arg(i, 2).replace(' ', '0')
                    .arg(QSettings().value("inbound/fileExt", "12000").toString());
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
      case 0: box = (CamBox*)ui->groupBox_1; break;
      case 1: box = (CamBox*)ui->groupBox_2; break;
      case 2: box = (CamBox*)ui->groupBox_3; break;
      case 3: box = (CamBox*)ui->groupBox_4; break;
      case 4: box = (CamBox*)ui->groupBox_5; break;
      case 5: box = (CamBox*)ui->groupBox_6; break;
      case 6: box = (CamBox*)ui->groupBox_7; break;
      case 7: box = (CamBox*)ui->groupBox_8; break;
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
        KradClient::anyCommand(QStringList()
                               << "record"
                               << "audiovideo"
                               << QString("%1/streaming/%2/aufnahmen/%3.webm").arg(QDir::homePath()).arg(startUp.toString("yyyy-MM-dd_hh-mm-ss")).arg(QDateTime().currentDateTime().toString("yyyy-MM-dd_hh-mm-ss"))
                               << "vp8vorbis"
                               << "960" << "540"
                               << QSettings().value("outbound/record_bitrate", "400").toString()
                               << "0.9");
        qDebug() << "Record id is" << KradClient::getRecordId();
    } else {
        KradClient::deleteStream(KradClient::getRecordId());
    }
}

void MainWindow::transmitButtonToggled(bool checked)
{
    if (checked) {
        KradClient::anyCommand(QStringList()
                               << "transmit"
                               << "audiovideo"
                               << QSettings().value("outbound/host", "127.0.0.1").toString()
                               << QSettings().value("outbound/port", "12000").toString()
                               << QString("/%1").arg(QSettings().value("outbound/mount", "jukuz.webm").toString())
                               << QSettings().value("outbound/pass", "pass").toString()
                               << "vp8vorbis"
                               << "960" << "540"
                               << QSettings().value("outbound/transmit_bitrate", "400").toString()
                               << "0.9");
        qDebug() << "Transmit id is" << KradClient::getTransmitId();
    } else {
        KradClient::deleteStream(KradClient::getTransmitId());
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

        textBgSpriteId = KradClient::addSprite(QString("%1/streaming/sprites/textsprite-960x540-scribble-alpha.png").arg(QDir::homePath()));
        textSpriteId   = KradClient::addSprite(fileName);
    } else {
        ui->textButton->setText("Text aktivieren");
        KradClient::delSprite(textBgSpriteId);
        KradClient::delSprite(textSpriteId);
    }
}

void MainWindow::logoButtonToggled(bool checked)
{
    if (checked) {
        logoSpriteId = KradClient::addSprite(QString("%1/streaming/sprites/bdv_logos.png").arg(QDir::homePath()));
    } else {
        KradClient::delSprite(logoSpriteId);
    }
}

void MainWindow::fadeInOneFadeOutOther(QObject *fadeInBox)
{
    foreach (QObject* boxObject, allCamBoxes) {
        CamBox* box = (CamBox*) boxObject;
        if (box == fadeInBox) {
            box->fadeStart(50, 50);
        } else {
            box->fadeStart(-50, 50);
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
