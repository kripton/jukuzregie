#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QDateTime>
#include <QSettings>
#include <QProcess>
#include <QDir>
#include <QUdpSocket>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QGraphicsOpacityEffect>
#include <QByteArray>
#include <QNetworkInterface>

#include "cambox.h"
#include "jackthread.h"
#include "audioappsrc.h"
#include "videoappsrc.h"

#include <QGlib/Connect>
#include <QGst/Element>
#include <QGst/ElementFactory>
#include <QGst/Pad>
#include <QGst/GhostPad>
#include <QGst/Bin>
#include <QGst/Pipeline>
#include <QGst/Bus>
#include <QGst/Message>
#include <QGst/Ui/VideoWidget>


namespace Ui {
class MainWindow;
}

struct camBoxMgmtData {
    QGraphicsPixmapItem* pixmapItem;
    QGraphicsOpacityEffect* opacityEffect;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void start();

    JackThread* worker;
    QDateTime startUp;

public slots:
    void midiEvent(char c0, char c1, char c2);

private slots:
    void recordButtonToggled(bool checked);
    void transmitButtonToggled(bool checked);
    void textButtonToggled(bool checked);
    void logoButtonToggled(bool checked);

    // Events by CamBoxes
    void fadeMeInHandler();
    void newOpacityHandler(qreal newValue);
    void newVideoFrame(QImage image);

    void newNotifyDatagram();

    void broadcastSourceInfo();

    void prepareAudioData(uint length, char *data);
    void prepareVideoData(uint length);

private:
    Ui::MainWindow *ui;
    QProcess* process;
    QStringList arguments;
    QList<QObject*> allCamBoxes;

    QList<QUdpSocket*> notifySockets;

    QGst::CapsPtr rawvideocaps;
    QGst::CapsPtr rawaudiocaps;

    QGst::PipelinePtr audioPipe;

    AudioAppSrc* audioSrc_main;
    AudioAppSrc* audioSrc_monitor;

    VideoAppSrc* videoSrc;

    QGraphicsScene scene;

    void onBusMessage(const QGst::MessagePtr & message);
    void startupApplications();
    void processNotifyDatagram(QByteArray datagram, QHostAddress senderHost, quint16 senderPort);
    void setOnAirLED(QObject *boxObject, bool newState);
};

#endif // MAINWINDOW_H
