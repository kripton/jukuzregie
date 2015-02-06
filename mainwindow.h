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
#include <QGraphicsProxyWidget>
#include <QMovie>
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

#include "nanoKontrol2.h"

#include "cambox.h"
#include "jackthread.h"
#include "audioappsrc.h"
#include "videoappsrc.h"

#include "camconnectdialog.h"
#include "videoadjustmentdialog.h"
#include "videoeffectdialog.h"

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
    void handleMidiEvent(char c0, char c1, char c2);

private slots:
    // UI events
    void textButtonToggled(bool checked);
    void logoButtonToggled(bool checked);
    void clockDisplayCheckboxToggled(bool checked);
    void selectNewLogoFile();
    void editTextFont();
    void selectNewTextBackground();
    void updateClockDisplay();

    // Events by CamBoxes (and VideoPlayer maybe)
    void fadeMeInHandler(bool fadeOutOthers = true, MediaSourceBase *sourceOverride = NULL);
    void newOpacityHandler(qreal newValue);
    void newVolumeHandler(qreal newValue);
    void newPrelistenHandler(bool newState);
    void newVideoFrame(QImage image);
    void loopChangedHandler(bool newState);
    void stateChangedHandler(QGst::State newState);
    void playButtonBlinkTimerTimeout();

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
    QLabel* logoLabel;
    QGraphicsProxyWidget* logoItem;
    QGraphicsOpacityEffect logoOpacityEffect;

    QFont textFont;
    QLabel* textSpriteLabel;
    QGraphicsProxyWidget* textSpriteItem;
    QGraphicsOpacityEffect textSpriteOpacityEffect;
    QGraphicsTextItem* textItem;

    QGraphicsTextItem* clockDisplayTextItem;
    QTimer clockDisplayTimer;

    CamConnectDialog* camConnectDialog;
    VideoAdjustmentDialog* videoAdjustmentDialog;
    VideoEffectDialog* videoEffectDialog;

    QTimer playButtonBlinkTimer;
    bool playButtonLEDState;

    void onBusMessage(const QGst::MessagePtr & message);
    void processNotifyDatagram(QByteArray datagram, QHostAddress senderHost, quint16 senderPort);
    void setOnAirLED(MediaSourceBase *boxObject, bool newState);
    void setLed(unsigned char num, bool newState);
};

#endif // MAINWINDOW_H
