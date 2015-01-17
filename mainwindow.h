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
#include <QFileDialog>
#include <QFontDialog>

#include <QGlib/Connect>
#include <QGst/Parse>
#include <QGst/Pipeline>
#include <QGst/Bus>
#include <QGst/Message>

#include "cambox.h"
#include "jackthread.h"
#include "audioappsrc.h"
#include "videoappsrc.h"
#include "camconnectdialog.h"

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

    JackThread* worker;
    QDateTime startUp;

public slots:
    void midiEvent(char c0, char c1, char c2);

private slots:
    // UI events
    void textButtonToggled(bool checked);
    void logoButtonToggled(bool checked);
    void selectNewLogoFile();
    void editTextFont();
    void selectNewTextBackground();

    // Events by CamBoxes
    void fadeMeInHandler(bool fadeOutOthers = true);
    void newOpacityHandler(qreal newValue);
    void newVideoFrame(QImage image);

    // UDP stuff
    void newNotifyDatagram();
    void broadcastSourceInfo();

    void startCam(CamBox* cam, QHostAddress host, quint16 port);

    // Audio and Video stuff
    void prepareAudioData(uint length, char* data);
    void prepareVideoData(uint length, char* data);

protected:
    bool eventFilter(QObject *obj, QEvent *ev);

private:
    Ui::MainWindow *ui;
    QProcess* process;
    QStringList arguments;

    QList<CamBox*> allCamBoxes;
    QList<MediaSourceBase*> allSources;

    QList<QUdpSocket*> notifySockets;

    QString rawvideocaps;
    QString rawaudiocaps;

    QGst::PipelinePtr outputPipe;

    AudioAppSrc* audioSrc_main;
    AudioAppSrc* audioSrc_monitor;

    VideoAppSrc* videoSrc;

    QGraphicsScene scene;
    QGraphicsPixmapItem* logoItem;
    QGraphicsOpacityEffect logoOpacityEffect;

    QFont textFont;
    QGraphicsPixmapItem* textSpriteItem;
    QGraphicsOpacityEffect textSpriteOpacityEffect;
    QGraphicsTextItem* textItem;

    CamConnectDialog* camConnectDialog;

    void onBusMessage(const QGst::MessagePtr & message);
    void processNotifyDatagram(QByteArray datagram, QHostAddress senderHost, quint16 senderPort);
    void setOnAirLED(QObject *boxObject, bool newState);
};

#endif // MAINWINDOW_H
