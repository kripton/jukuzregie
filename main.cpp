#include "mainwindow.h"
#include <QApplication>

MainWindow* wP;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("JuKuZRegie");

    MainWindow w;

    wP = &w;

    w.show();

    w.start();

    return a.exec();
}

int process_wrapper(jack_nframes_t nframes, void *arg) {
    return wP->worker->process(nframes, arg);
}
