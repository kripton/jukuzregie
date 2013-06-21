#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QDateTime>
#include "cambox.h"
#include <phonon/VideoPlayer>
#include <jackthread.h>
#include "nanoKontrol2.h"

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

    Phonon::VideoPlayer* player;
    JackThread* worker;

public slots:
    void midiEvent(char c0, char c1, char c2);

private slots:
    void recordButtonToggled(bool checked);
    void transmitButtonToggled(bool checked);
    void textButtonToggled(bool checked);

private:
    Ui::MainWindow *ui;
    QProcess* westonprocess;
    QProcess* process;
    QStringList arguments;

    void startupApplications();
};

#endif // MAINWINDOW_H
