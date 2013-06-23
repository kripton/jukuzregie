#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->groupBox->setMainWindow(this);
    ui->groupBox_2->setMainWindow(this);
    ui->groupBox_3->setMainWindow(this);
    ui->groupBox_4->setMainWindow(this);

    startUp = QDateTime::currentDateTime();
    QDir().mkpath(QString("%1/streaming/%2/aufnahmen/").arg(QDir::homePath()).arg(startUp.toString("yyyy-MM-dd_hh-mm-ss")));

    startupApplications();

    // Start the JACK-thread
    QThread* thread = new QThread;
    worker = new JackThread();
    worker->moveToThread(thread);
    //connect(worker, SIGNAL(error(QString)), this, SLOT(errorString(QString)));
    connect(thread, SIGNAL(started()), worker, SLOT(setup()));
    connect(worker, SIGNAL(finished()), thread, SLOT(quit()));
    connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

    connect(worker, SIGNAL(midiEvent(char, char, char)), this, SLOT(midiEvent(char, char, char)));

    connect(ui->recordButton, SIGNAL(toggled(bool)), this, SLOT(recordButtonToggled(bool)));
    connect(ui->transmitButton, SIGNAL(toggled(bool)), this, SLOT(transmitButtonToggled(bool)));
    connect(ui->textButton, SIGNAL(toggled(bool)), this, SLOT(textButtonToggled(bool)));

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

    qDebug() << "Starting up weston ...";
    westonprocess = new QProcess();
    westonprocess->setProcessEnvironment(env);
    westonprocess->start("weston", QStringList() << "--idle-time=0");
    westonprocess->waitForStarted();
    qDebug() << "Weston has been started";

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
    KradClient::anyCommand(QStringList() << "display");
    KradClient::anyCommand(QStringList() << "output" << "jack");
    qDebug() << "KRAD has been started";
}

void MainWindow::start() {
    //Phonon::MediaSource* source2 = new Phonon::MediaSource("http://127.0.0.1:12000/cam_01.ogg");
    //Phonon::MediaSource* source = new Phonon::MediaSource("/home/kripton/youtube/d9l0RArSpDc.webm");
    //Phonon::MediaSource* source3 = new Phonon::MediaSource("/home/kripton/youtube/dav6_clickup.mp4");
    //Phonon::MediaSource* source4 = new Phonon::MediaSource("/home/kripton/youtube/Flight1549CrashAndRescue.ogg");

    ui->groupBox->iInfo.baseUrl = QUrl("http://127.0.0.1:12000/");
    ui->groupBox->setMountName("cam_01.ogg");

    ui->groupBox_2->iInfo.baseUrl = QUrl("http://127.0.0.1:12000/");
    ui->groupBox_2->setMountName("cam_02.ogg");

    ui->groupBox_3->iInfo.baseUrl = QUrl("http://127.0.0.1:12000/");
    ui->groupBox_3->setMountName("cam_03.ogg");

    ui->groupBox_4->iInfo.baseUrl = QUrl("http://127.0.0.1:12000/");
    ui->groupBox_4->setMountName("cam_04.ogg");
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

    // Determine action by 1st nibble
    switch (c1 & 0xf0) {
      case 0x00: // Fader
        box->setVideoOpacity(opacity);
        return;

      case 0x10: // Knob
        return;

      case 0x20: // Solo
        if (c2 == 0) return;
        box->setPreListen(!box->getPreListen());
        return;

      case 0x30: // Mute
        return;

      case 0x40: // Rec
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
                               << "400" << "0.9");
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
                               << "127.0.0.1"
                               << "12000"
                               << "/jukuz.webm"
                               << "pass"
                               << "vp8vorbis"
                               << "960" << "540"
                               << "400" << "0.9");
        qDebug() << "Transmit id is" << KradClient::getTransmitId();
    } else {
        KradClient::deleteStream(KradClient::getTransmitId());
    }
}

void MainWindow::textButtonToggled(bool checked)
{
    if (checked) {
        ui->textButton->setText("Text deaktivieren");
        KradClient::anyCommand(QStringList()
                               << "addsprite"
                               << QString("%1/streaming/sprites/textsprite-960x540-scribble-alpha.png").arg(QDir::homePath())
                               << "0" << "0"
                               << "1"
                               );
        KradClient::anyCommand(QStringList()
                               << "addtext"
                               << "HELLO"
                               << "155" << "385"
                               << "4" << "80.0f" << "1.0f" << "0.0f"
                               << "255" << "255" << "255"
                               );
    } else {
        ui->textButton->setText("Text aktivieren");
        KradClient::anyCommand(QStringList() << "rmsprite" << "0");
        KradClient::anyCommand(QStringList() << "rmtext" << "0");
    }
}
