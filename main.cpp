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

    int width = 640;
    int height = 360;
    int fps = 25;

    if ((QApplication::arguments().indexOf("-s") != -1) &&
        (QApplication::arguments().size() > QApplication::arguments().indexOf("-s")))
    {
        QString sizeString = QApplication::arguments().at(QApplication::arguments().indexOf("-s") + 1);
        if (sizeString.contains("@"))
        {
            fps = sizeString.split("@").at(1).toInt();
            sizeString = sizeString.split("@").at(0);
        }
        if (sizeString.contains("x"))
        {
            width = sizeString.split("x").at(0).toInt();
            height = sizeString.split("x").at(1).toInt();
        }
    }

    qDebug() << "OUTPUT VIDEO FORMAT:" << width << "x" << height << "@" << fps;

    MainWindow w(0, width, height, fps);

    wP = &w;
    w.show();

    return a.exec();
}

int process_wrapper(jack_nframes_t nframes, void *arg) {
    return wP->worker->process(nframes, arg);
}
