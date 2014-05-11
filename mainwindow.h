#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QDateTime>
#include <QSettings>
#include <QProcess>
#include <QDir>
#include <QUdpSocket>
#include "cambox.h"
#include <jackthread.h>

#include <QGst/Element>
#include <QGst/ElementFactory>
#include <QGst/Pad>
#include <QGst/GhostPad>
#include <QGst/Bin>
#include <QGst/Pipeline>
#include <QGst/Ui/VideoWidget>

namespace Ui {
class MainWindow;
}

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
    void newPreListenChangedHandler(bool newState);
    void newOpacityHandler(qreal newValue);
    void newVolumeHandler(qreal newValue);

    void newNotifyDatagram();

    void broadcastSourceInfo();

private:
    Ui::MainWindow *ui;
    QProcess* process;
    QStringList arguments;
    QList<QObject*> allCamBoxes;
    QHash<QObject*, QGst::PadPtr> boxMixerPads;

    QUdpSocket* notifySocket;

    QGst::CapsPtr rawvideocaps;
    QGst::CapsPtr rawaudiocaps;
    QGst::PipelinePtr Pipeline;
    QGst::ElementPtr VideoSinkPreview;
    QGst::ElementPtr VideoMixer;
    QGst::ElementPtr VideoMixerTee;

    void startupApplications();
    void processNotifyDatagram(QByteArray datagram, QHostAddress senderHost, quint16 senderPort);
    void setOnAirLED(QObject *boxObject, bool newState);
};

#endif // MAINWINDOW_H
