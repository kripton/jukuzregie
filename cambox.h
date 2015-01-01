#ifndef CAMBOX_H
#define CAMBOX_H

#include <QDebug>
#include <QGroupBox>
#include <QUrl>
#include <QTimer>
#include <QMutex>
#include <QWaitCondition>
#include <QImage>
#include <QHostAddress>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QQueue>

#include <QGlib/Connect>
#include <QGst/Parse>
#include <QGst/Pipeline>
#include <QGst/Bus>
#include <QGst/Message>

#include "mediasourcebase.h"

#include "tcpappsrc.h"
#include "audioappsink.h"
#include "videoappsink.h"

namespace Ui {
class CamBox;
}

class CamBox : public MediaSourceBase
{
    Q_OBJECT

public:
    explicit CamBox(QWidget *parent = 0);
    ~CamBox();

signals:

public slots:
    void startCam(QHostAddress host, quint16 port, QString videocaps, QString audiocaps); // start playing from a source
    void setDumpDir(QString dir);           // Specify in which directory the incoming stream should be archived to
    void disconnectSource();

protected slots:
    void sourceOnline();
    void sourceOffline();
    void updateTitle();

private:
    Ui::CamBox *ui;
    QString dumpDir;

    void onBusMessage(const QGst::MessagePtr & message);

    TcpAppSrc m_tcpsrc;
};

#endif // CAMBOX_H
