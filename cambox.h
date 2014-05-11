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

    QString name;                           // The name of this cambox. Like "cam_02"
    bool getPreListen();                    // read-only public access to state of Pre-Listen button
    bool getCamOnline();                    // read-only public access to camOnline
    QHash<QString, QString> sourceInfo();

signals:
    void fadeMeIn();                        // emitted for parent when the GO-Button is clicked
    void newPreListen(bool newState);       // emitted for parent when the state of the Pre-Listen value changed
    void newOpacity(qreal newOpacity);      // emitted for parent when the opacity changed
    void newVolume(qreal newVolume);        // emitted for parent when the volume changed

public slots:
    void setVideoOpacity(qreal opacity);    // set the opacity slider to a new value
    void setVolume(qreal volume);           // set the volume fader to a new value
    void setPreListen(bool value);          // write-only public access to state of Pre-Listen button
    QGst::BinPtr startCam(QHostAddress host, quint16 port, QGst::CapsPtr videocaps, QGst::CapsPtr audiocaps); // start playing from a source

private slots:
    void opcatiyFaderChanged();             // called when the opacity-fader got changed
    void volumeFaderChanged();              // called when the volume-fader got changed
    void sourceOnline();                    // UI cleanups/defaults after the source has come online
    void sourceOffline();                   // UI cleanups/defaults after the source has gone offline
    void updateBackground();                // Update the background color and title of this cambox
    void preListenButtonToggled(bool checked);// Internal slot to handle Pre-Listen button state changes
    void goButtonClicked();                 // Internal slot to handle GO-Button clicks

private:
    // General management stuff
    Ui::CamBox *ui;

    bool camOnline;

    qreal opacity;
    qreal volume;

    QGst::BinPtr bin;
};

#endif // CAMBOX_H
