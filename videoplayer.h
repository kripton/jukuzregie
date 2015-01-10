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
    explicit VideoPlayer(QWidget *parent = 0);
    ~VideoPlayer();
    void init(QString videocaps, QString audiocaps);

private slots:
    void selectFolder();
    void newFileSelected(QString newFile);
    void goButtonClicked();

    void onStateChanged();
    void onPositionChanged();
    void updatePositionLabel(QTime len, QTime pos);
    void setPosition(int position);

    void play();
    void pause();
    void stop();

private:
    Ui::VideoPlayer *ui;
    QString currentDir;

    QString videocaps;
    QString audiocaps;

    QTimer positionTimer;

    QTime length() const;
    QTime position() const;
    void setPosition(const QTime & pos);
    void onBusMessage(const QGst::MessagePtr & message);
    void handlePipelineStateChange(const QGst::StateChangedMessagePtr & scm);
};

#endif // VIDEOPLAYER_H
