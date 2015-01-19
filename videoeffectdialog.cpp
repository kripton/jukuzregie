#include "videoeffectdialog.h"
#include "ui_videoeffectdialog.h"

VideoEffectDialog::VideoEffectDialog(QList<MediaSourceBase*> sources, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::VideoEffectDialog)
{
    ui->setupUi(this);
    this->sources = sources;

    ui->sourcesListWidget->clear();
    foreach (MediaSourceBase* source, sources)
    {
        ui->sourcesListWidget->addItem(source->getId());
    }

    QStringList effects;
    effects << "None" << "Bulge" << "Circle" << "Diffuse" << "Fisheye" << "Kaleidoscope" << "Marble" << "Mirror"
            << "Pinch" << "Sphere" << "Square" << "Stretch" << "Tunnel" << "Twirl" << "Waterripple"
            << "AgingTV" << "DiceTV" << "EdgeTV" << "OpTV" << "QuarkTV" << "RadioacTV"
            << "RevTV" << "RippleTV" << "ShagadelicTV" << "StreakTV" << "VertigoTV" << "WarpTV";
    ui->effectListWidget->addItems(effects);
}

VideoEffectDialog::~VideoEffectDialog()
{
    delete ui;
}

MediaSourceBase *VideoEffectDialog::getSourceById(QString id)
{
    foreach (MediaSourceBase* source, sources)
    {
        if (source->getId() == id)
        {
            return source;
        }
    }

    return NULL;
}
