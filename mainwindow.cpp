#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

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

    thread->start();
}

MainWindow::~MainWindow()
{
    delete ui;
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

    switch (c1) {
      case KNK2_Fader1:
        ((CamBox*)ui->groupBox)->setVideoOpacity(opacity);
        return;

      case KNK2_Fader2:
        ((CamBox*)ui->groupBox_2)->setVideoOpacity(opacity);
        return;

      case KNK2_Fader3:
        ((CamBox*)ui->groupBox_3)->setVideoOpacity(opacity);
        return;

      case KNK2_Fader4:
        ((CamBox*)ui->groupBox_4)->setVideoOpacity(opacity);
        return;
    }
}
