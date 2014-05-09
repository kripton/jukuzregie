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
    void fadeInOneFadeOutOther(QObject* fadeInBox);
    void preListenChangedHandler(QObject* sender, bool newState);
    void onAirInfoHandler(QObject* sender, bool newState);

    void newNotifyDatagram();

    void broadcastSourceInfo();

private:
    Ui::MainWindow *ui;
    QProcess* process;
    QStringList arguments;
    QList<QObject*> allCamBoxes;

    QUdpSocket* notifySocket;

    QGst::CapsPtr rawvidcaps;
    QGst::PipelinePtr Pipeline;
    QGst::ElementPtr VideoSinkPreview;
    QGst::ElementPtr VideoMixer;
    QGst::ElementPtr VideoMixerTee;

    void startupApplications();
};

#endif // MAINWINDOW_H
