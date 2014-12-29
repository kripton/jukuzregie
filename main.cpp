#include "mainwindow.h"
#include <QApplication>
#include <QGst/Init>

MainWindow* wP;

int main(int argc, char *argv[])
{
    QApplication::setOrganizationName("jukuz");
    QApplication::setOrganizationDomain("jukuz.de");
    QApplication::setApplicationName("LiveStreamRegie");
    QApplication a(argc, argv);
    QGst::init(&argc, &argv);

    MainWindow w;

    wP = &w;
    w.show();

    return a.exec();
}

int process_wrapper(jack_nframes_t nframes, void *arg) {
    return wP->worker->process(nframes, arg);
}
