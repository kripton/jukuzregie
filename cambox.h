#ifndef CAMBOX_H
#define CAMBOX_H

#include <QGroupBox>
#include <QUrl>
#include <QTimer>
#include <QMutex>
#include <QWaitCondition>
#include "mainwindow.h"

#include <QGst/Ui/VideoWidget>

namespace Ui {
class CamBox;
}

class CamBox : public QGroupBox
{
    Q_OBJECT

public:
    explicit CamBox(QWidget *parent = 0);
    ~CamBox();

    bool getPreListen();
    void setMainWindow(QObject* mainWin);
    bool isSourceOnline();

    QGst::Ui::VideoWidget* VideoWidget();

signals:
    void fadeMeIn(QObject* sender);
    void preListenChanged(QObject* sender, bool newState);
    void onAirInfo(QObject* sender, bool newState);

public slots:
    void setName(QString name);
    void setVideoOpacity(qreal opacity);
    void setAudioVolume(qreal volume);
    void setPreListen(bool value);
    void fadeStart(qint16 stepSize, qint16 interval);

private slots:
//    void opcatiyFaderChanged();
//    void volumeFaderChanged();
    void sourceOnline();
    void sourceOffline();
    void updateBackGround(); // background of UI groupbox
    void mute();
    void unmute();
    void MonitorPushButtonToggled(bool checked);
    void fadeTimeEvent();
    void goButtonClicked();

private:
    // General management stuff
    Ui::CamBox *ui;

    QString name;
    bool camOnline;

    QTimer timer;
    QObject* mainWin;

    qint16 fadeStepSize;
    QTimer* fadeTimer;
};

#endif // CAMBOX_H
