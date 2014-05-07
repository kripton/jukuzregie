#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QDateTime>
#include <QSettings>
#include <QProcess>
#include <QDir>
#include "cambox.h"
#include <jackthread.h>

#include <QGst/Element>
#include <QGst/ElementFactory>
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

private:
    Ui::MainWindow *ui;
    QProcess* process;
    QStringList arguments;
    QList<QObject*> allCamBoxes;

    qint16 logoSpriteId;
    qint16 textBgSpriteId;
    qint16 textSpriteId;

    QGst::CapsPtr rawvidcaps;
    QGst::PipelinePtr Pipeline;
    QGst::ElementPtr VideoSinkPreview;

    void startupApplications();
};

#endif // MAINWINDOW_H
