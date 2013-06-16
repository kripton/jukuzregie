#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include "cambox.h"
#include <phonon/VideoPlayer>
#include <jackthread.h>
#include <QProcess>

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

private:
    Ui::MainWindow *ui;
    QProcess *process;
    QStringList arguments;
};

#endif // MAINWINDOW_H
