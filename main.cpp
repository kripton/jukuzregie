#include "mainwindow.h"
#include <QApplication>

MainWindow* wP;

int main(int argc, char *argv[])
{
    QApplication::setOrganizationName("jukuz");
    QApplication::setOrganizationDomain("jukuz.de");
    QApplication::setApplicationName("LiveStreamRegie");
    QApplication a(argc, argv);

    QSettings settings;
    //settings.setValue("general/krad_station_name", "jukuz");
    settings.sync();

    MainWindow w;

    wP = &w;
    w.show();
    w.start();

    return a.exec();
}

int process_wrapper(jack_nframes_t nframes, void *arg) {
    return wP->worker->process(nframes, arg);
}
