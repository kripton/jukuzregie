#include "videoplayer.h"
#include "ui_videoplayer.h"

VideoPlayer::VideoPlayer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::VideoPlayer)
{
    ui->setupUi(this);
    ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    ui->playButton->setIconSize(QSize(32, 32));
    ui->pauseButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
    ui->pauseButton->setIconSize(QSize(32, 32));
    ui->stopButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    ui->stopButton->setIconSize(QSize(32, 32));

    ui->dirSelectButton->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    ui->dirSelectButton->setIconSize(QSize(32, 32));
}

VideoPlayer::~VideoPlayer()
{
    delete ui;
}
