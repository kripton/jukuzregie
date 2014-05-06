#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QDateTime>
#include <QSettings>
#include <QProcess>
#include "cambox.h"
#include <jackthread.h>

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
    QProcess* westonprocess;
    QProcess* process;
    QStringList arguments;
    QList<QObject*> allCamBoxes;

    qint16 logoSpriteId;
    qint16 textBgSpriteId;
    qint16 textSpriteId;

    void startupApplications();
};

#endif // MAINWINDOW_H
