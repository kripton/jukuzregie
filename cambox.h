#ifndef CAMBOX_H
#define CAMBOX_H

#include <QGroupBox>
#include <QUrl>
#include <QTimer>
#include <QMutex>
#include <QWaitCondition>
#include "mainwindow.h"

#include <QGst/Ui/VideoWidget>
#include <QGst/Bin>

namespace Ui {
class CamBox;
}

class CamBox : public QGroupBox
{
    Q_OBJECT

public:
    explicit CamBox(QWidget *parent = 0);
    ~CamBox();

    bool getPreListen();
    void setMainWindow(QObject* mainWin);
    bool isSourceOnline();

    QGst::Ui::VideoWidget* VideoWidget();

signals:
    void fadeMeIn(QObject* sender);
    void newPreListen(QObject* sender, bool newState);
    void onAirInfo(QObject* sender, bool newState);
    void newOpacity(QObject* sender, qreal newOpacity);
    void newVolume(QObject* sender, qreal newVolume);

public slots:
    void setName(QString name);
    void setVideoOpacity(qreal opacity);
    void setVolume(qreal volume);
    void setPreListen(bool value);

private slots:
    void opcatiyFaderChanged();
    void volumeFaderChanged();
    void sourceOnline();
    void sourceOffline();
    void updateBackground();
    void monitorButtonToggled(bool checked);
    void goButtonClicked();

private:
    // General management stuff
    Ui::CamBox *ui;

    QString name;
    bool camOnline;

    QObject* mainWin;

    qreal opacity;
    qreal volume;

    QGst::Bin bin;
};

#endif // CAMBOX_H
