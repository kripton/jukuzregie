#ifndef KRADCLIENT_H
#define KRADCLIENT_H

#include <QObject>
#include <QStringList>
#include <QProcess>
#include <QUrl>
#include <QDir>
#include <QDebug>
#include <QSettings>

class KradClient : public QObject
{
    Q_OBJECT
private:
    QSettings* settings;

public:
    explicit KradClient(QObject *parent = 0);
    static void anyCommand(QStringList params);
    static void launch();
    static void kill();
    static qint16 addSprite(QString filename);
    static bool delSprite(qint16 id);
    static qint16 playStream(QUrl streamUrl);
    static bool deleteStream(qint16 id);
    static qint16 getRecordId();
    static qint16 getTransmitId();
    static void ls();
};

#endif // KRADCLIENT_H
