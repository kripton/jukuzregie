#include "camconnectdialog.h"
#include "ui_camconnectdialog.h"

CamConnectDialog::CamConnectDialog(QList<CamBox *> cams, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CamConnectDialog)
{
    ui->setupUi(this);
    this->cams = cams;

    ui->camListWidget->clear();
    foreach (CamBox* box, cams)
    {
        ui->camListWidget->addItem(box->getId());
    }
}

CamConnectDialog::~CamConnectDialog()
{
    delete ui;
}

void CamConnectDialog::accept()
{
    if (ui->camListWidget->selectedItems().length() != 1)
    {
        return;
    }

    CamBox* box = getCamById(ui->camListWidget->selectedItems().at(0)->text());
    if (box == NULL)
    {
        return;
    }

    bool ok = false;
    quint16 port = ui->portLineEdit->text().toUShort(&ok);

    if (!ok)
    {
        return;
    }

    emit connectCam(box, QHostAddress(ui->hostLineEdit->text()), port);

    hide();
}

CamBox *CamConnectDialog::getCamById(QString id)
{
    foreach (CamBox* box, cams)
    {
        if (box->getId() == id)
        {
            return box;
        }
    }

    return NULL;
}

