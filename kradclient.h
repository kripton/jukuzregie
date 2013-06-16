#ifndef KRADCLIENT_H
#define KRADCLIENT_H

#include <QObject>
#include <QStringList>
#include <QProcess>
#include <QUrl>
#include <QDebug>

class KradClient : public QObject
{
    Q_OBJECT
public:
    explicit KradClient(QObject *parent = 0);
    static void anyCommand(QStringList params);
    static void startUp();
    static qint16 playStream(QUrl streamUrl);
    static bool deleteStream(qint16 id);
};

#endif // KRADCLIENT_H
