#include "videoadjustmentdialog.h"
#include "ui_videoadjustmentdialog.h"

VideoAdjustmentDialog::VideoAdjustmentDialog(QList<MediaSourceBase*> sources, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::VideoAdjustmentDialog)
{
    ui->setupUi(this);
    this->sources = sources;

    ui->sourcesListWidget->clear();
    foreach (MediaSourceBase* source, sources)
    {
        ui->sourcesListWidget->addItem(source->getId());
    }

    connect(ui->sourcesListWidget, SIGNAL(currentTextChanged(QString)), this, SLOT(newSourceSelected(QString)));

    connect(ui->gammaResetButton, SIGNAL(clicked()), this, SLOT(resetGamma()));
    connect(ui->brightnessResetButton, SIGNAL(clicked()), this, SLOT(resetBrightness()));
    connect(ui->contrastResetButton, SIGNAL(clicked()), this, SLOT(resetContrast()));
    connect(ui->hueResetButton, SIGNAL(clicked()), this, SLOT(resetHue()));
    connect(ui->saturationResetButton, SIGNAL(clicked()), this, SLOT(resetSaturation()));

    connect(ui->gammaSlider, SIGNAL(valueChanged(int)), this, SLOT(gammaChanged(int)));
    connect(ui->brightnessSlider, SIGNAL(valueChanged(int)), this, SLOT(brightnessChanged(int)));
    connect(ui->contrastSlider, SIGNAL(valueChanged(int)), this, SLOT(contrastChanged(int)));
    connect(ui->hueSlider, SIGNAL(valueChanged(int)), this, SLOT(contrastChanged(int)));
    connect(ui->saturationSlider, SIGNAL(valueChanged(int)), this, SLOT(saturationChanged(int)));

    blockChanges = false;
}

VideoAdjustmentDialog::~VideoAdjustmentDialog()
{
    delete ui;
}

void VideoAdjustmentDialog::newSourceSelected(QString newSource)
{
    MediaSourceBase* source = getSourceById(newSource);
    if (source == NULL)
    {
        return;
    }

    blockChanges = true;
    ui->gammaSlider->setValue(source->getGamma() * 100);
    ui->brightnessSlider->setValue(source->getBrightness() * 100);
    ui->contrastSlider->setValue(source->getContrast() * 100);
    ui->hueSlider->setValue(source->getHue() * 100);
    ui->saturationSlider->setValue(source->getSaturation() * 100);
    blockChanges = false;
}

void VideoAdjustmentDialog::resetGamma()
{
    ui->gammaSlider->setValue(100);
}

void VideoAdjustmentDialog::resetBrightness()
{
    ui->brightnessSlider->setValue(0);
}

void VideoAdjustmentDialog::resetContrast()
{
    ui->contrastSlider->setValue(100);
}

void VideoAdjustmentDialog::resetHue()
{
    ui->hueSlider->setValue(0);
}

void VideoAdjustmentDialog::resetSaturation()
{
    ui->saturationSlider->setValue(100);
}

void VideoAdjustmentDialog::gammaChanged(int newValue)
{
    if ((ui->sourcesListWidget->selectedItems().length() != 1) || blockChanges)
    {
        return;
    }

    MediaSourceBase* source = getSourceById(ui->sourcesListWidget->selectedItems().at(0)->text());
    if (source == NULL)
    {
        return;
    }

    source->setGamma(newValue / 100.0);
}

void VideoAdjustmentDialog::brightnessChanged(int newValue)
{
    if ((ui->sourcesListWidget->selectedItems().length() != 1) || blockChanges)
    {
        return;
    }

    MediaSourceBase* source = getSourceById(ui->sourcesListWidget->selectedItems().at(0)->text());
    if (source == NULL)
    {
        return;
    }

    source->setBrightness(newValue / 100.0);
}

void VideoAdjustmentDialog::contrastChanged(int newValue)
{
    if ((ui->sourcesListWidget->selectedItems().length() != 1) || blockChanges)
    {
        return;
    }

    MediaSourceBase* source = getSourceById(ui->sourcesListWidget->selectedItems().at(0)->text());
    if (source == NULL)
    {
        return;
    }

    source->setContrast(newValue / 100.0);
}

void VideoAdjustmentDialog::hueChanged(int newValue)
{
    if ((ui->sourcesListWidget->selectedItems().length() != 1) || blockChanges)
    {
        return;
    }

    MediaSourceBase* source = getSourceById(ui->sourcesListWidget->selectedItems().at(0)->text());
    if (source == NULL)
    {
        return;
    }

    source->setHue(newValue / 100.0);
}

void VideoAdjustmentDialog::saturationChanged(int newValue)
{
    if ((ui->sourcesListWidget->selectedItems().length() != 1) || blockChanges)
    {
        return;
    }

    MediaSourceBase* source = getSourceById(ui->sourcesListWidget->selectedItems().at(0)->text());
    if (source == NULL)
    {
        return;
    }

    source->setSaturation(newValue / 100.0);
}

MediaSourceBase *VideoAdjustmentDialog::getSourceById(QString id)
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
