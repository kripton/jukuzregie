#ifndef CAMCONNECTDIALOG_H
#define CAMCONNECTDIALOG_H

#include <QDialog>
#include <QList>

#include "cambox.h"

namespace Ui {
class CamConnectDialog;
}

class CamConnectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CamConnectDialog(QList<CamBox*> cams, QWidget *parent = 0);
    ~CamConnectDialog();

public slots:
    void accept();

signals:
    void connectCam(CamBox* cam, QHostAddress host, quint16 port);

private:
    Ui::CamConnectDialog *ui;
    QList<CamBox*> cams;

    CamBox* getCamById(QString id);
};

#endif // CAMCONNECTDIALOG_H
