#ifndef CAMBOX_H
#define CAMBOX_H

#include <QGroupBox>
#include <QUrl>
#include <QTimer>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QMutex>
#include <QWaitCondition>
#include <phonon/VideoPlayer>
#include <phonon/MediaObject>
#include <phonon/AudioDataOutput>
#include <phonon/MediaSource>
#include <phonon/AudioOutput>
#include "audiometer.h"
#include "kradclient.h"
#include "mainwindow.h"

namespace Ui {
class CamBox;
}

struct IcecastInfo {
    bool sourceOnline;
    QUrl baseUrl;
};

struct VideoPort {
    qint16 id;
    qint16 pos_x;
    qint16 pos_y;
    qint16 width;
    qint16 height;
    qint16 crop_x;
    qint16 crop_y;
    qint16 crop_width;
    qint16 crop_height;
    qreal opacity;
    qreal rotation;

    qreal volume;
};

class CamBox : public QGroupBox
{
    Q_OBJECT

public:
    explicit CamBox(QWidget *parent = 0);
    ~CamBox();
    IcecastInfo iInfo;

    bool getPreListen();
    void setMainWindow(QObject* mainWin);
    bool isSourceOnline();

signals:
    void fadeMeIn(QObject* sender);

public slots:
    void setMountName(QString mountName);
    void setVideoOpacity(qreal opacity);
    void setKradVolume(qreal volume);
    void setPreListen(bool value);
    void fadeStart(qint16 stepSize, qint16 interval);

private slots:
    void _setVideoOpacity(qreal opacity);
    void _setKradVolume(qreal volume);

    void opcatiyFaderChanged();
    void volumeFaderChanged();
    void updateKradPort();
    void pollIcecastRequest();
    void sourceOnline();
    void sourceOffline();
    void updateBackGround();
    void mute();
    void unmute();
    void MonitorPushButtonToggled(bool checked);
    void fadeTimeEvent();
    void goButtonClicked();

private:
    // General management stuff
    Ui::CamBox *ui;
    QString mountName;
    QTimer timer;
    Phonon::AudioDataOutput* dataOutput;
    Phonon::MediaSource* mediaSource;
    QObject* mainWin;

    qint16 fadeStepSize;
    QTimer* fadeTimer;

    // KRAD related stuff
    VideoPort vPort;

    // Icecast related stuff
    QNetworkAccessManager qnam;
    QNetworkRequest request;
    QNetworkReply* reply;
    QProcess* icecastDumpProc;
};

#endif // CAMBOX_H
