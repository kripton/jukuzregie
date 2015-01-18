#include "videoeffectdialog.h"
#include "ui_videoeffectdialog.h"

VideoEffectDialog::VideoEffectDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::VideoEffectDialog)
{
    ui->setupUi(this);
    QStringList effects;
    effects << "None" << "Fisheye";
    ui->effectListWidget->addItems(effects);
}

VideoEffectDialog::~VideoEffectDialog()
{
    delete ui;
}
