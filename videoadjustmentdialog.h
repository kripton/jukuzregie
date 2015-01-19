#ifndef VIDEOADJUSTMENTDIALOG_H
#define VIDEOADJUSTMENTDIALOG_H

#include <QDialog>
#include <QList>

#include "mediasourcebase.h"

namespace Ui {
class VideoAdjustmentDialog;
}

class VideoAdjustmentDialog : public QDialog
{
    Q_OBJECT

public:
    explicit VideoAdjustmentDialog(QList<MediaSourceBase*> cams, QWidget *parent = 0);
    ~VideoAdjustmentDialog();

public slots:
    void newSourceSelected(QString newSource);

    void resetGamma();
    void resetBrightness();
    void resetContrast();
    void resetHue();
    void resetSaturation();

    void gammaChanged(int newValue);
    void brightnessChanged(int newValue);
    void contrastChanged(int newValue);
    void hueChanged(int newValue);
    void saturationChanged(int newValue);

private:
    Ui::VideoAdjustmentDialog *ui;
    QList<MediaSourceBase*> sources;
    bool blockChanges;

    MediaSourceBase* getSourceById(QString id);
};

#endif // VIDEOADJUSTMENTDIALOG_H
