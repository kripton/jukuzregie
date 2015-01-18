#ifndef VIDEOEFFECTDIALOG_H
#define VIDEOEFFECTDIALOG_H

#include <QDialog>

namespace Ui {
class VideoEffectDialog;
}

class VideoEffectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit VideoEffectDialog(QWidget *parent = 0);
    ~VideoEffectDialog();

private:
    Ui::VideoEffectDialog *ui;
};

#endif // VIDEOEFFECTDIALOG_H
