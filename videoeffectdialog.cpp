#include "videoeffectdialog.h"
#include "ui_videoeffectdialog.h"

VideoEffectDialog::VideoEffectDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::VideoEffectDialog)
{
    ui->setupUi(this);
}

VideoEffectDialog::~VideoEffectDialog()
{
    delete ui;
}
