#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QDebug>
#include <QGroupBox>
#include <QFileDialog>

#include <QGlib/Connect>
#include <QGst/Parse>
#include <QGst/Pipeline>
#include <QGst/Bus>
#include <QGst/Message>
#include <QGst/Query>
#include <QGst/Event>

#include "mediasourcebase.h"

#include "audioappsink.h"
#include "videoappsink.h"

namespace Ui {
class VideoPlayer;
}

class VideoPlayer : public MediaSourceBase
{
    Q_OBJECT

public:
    /// PROPERTIES ///
    bool getLoop();
    void setLoop(bool newState);

    /// METHODS ///
    explicit VideoPlayer(QWidget *parent = 0);
    ~VideoPlayer();
    void init(QString videocaps, QString audiocaps);

    void setPosition(const QTime & pos);

private slots:
    void selectFolder();
    void newFileSelected(QString newFile);
    void goButtonClicked();

    void onStateChanged();
    void onPositionChanged();
    void updatePositionLabel(QTime len, QTime pos);
    void setPosition(int position);

public slots:
    void play();
    void pause();
    void playOrPause();
    void stop();

signals:
    void loopChanged(bool newState);

private:
    Ui::VideoPlayer *ui;
    QString currentDir;

    QString videocaps;
    QString audiocaps;

    QTimer positionTimer;

    QTime length() const;
    QTime position() const;
    void onBusMessage(const QGst::MessagePtr & message);
    void handlePipelineStateChange(const QGst::StateChangedMessagePtr & scm);
};

#endif // VIDEOPLAYER_H
