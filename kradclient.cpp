#include "kradclient.h"

KradClient::KradClient(QObject *parent) :
    QObject(parent)
{
}

void KradClient::anyCommand(QStringList params)
{
    QProcess* process = new QProcess();
    params.prepend("jukuz");
    //arguments << "setport" << "3" << "180" << "40" << "160" << "120" << "0" << "0" << "160" << "120" << QString("%1").arg(opacity) << "0";
    qDebug() << params;
    process->start("/usr/bin/krad_radio", params);
    process->waitForFinished();
    process->~QProcess();
    process = NULL;
}

void KradClient::startUp()
{
}

qint16 KradClient::playStream(QUrl streamUrl) {
    qint16 retval = -1;
    bool parseOkay = false;

    // Play the stream
    QStringList params;
    params << "play" << streamUrl.host() << QString("%1").arg(streamUrl.port()) << streamUrl.path();
    anyCommand(params);

    QProcess* process = new QProcess();
    params.clear();
    params << "jukuz" << "ls";
    process->start("/usr/bin/krad_radio", params);
    process->waitForFinished();
    foreach (QString line, process->readAll().split('\n')) {
        if (!line.contains(streamUrl.path())) continue;
        retval = line.split(':')[1].trimmed().split(' ')[0].toInt(&parseOkay);
        if (!parseOkay) retval = -1;
    }

    process->~QProcess();
    process = NULL;

    return retval;
}

bool KradClient::deleteStream(qint16 id)
{
    qDebug() << "deleteStream" << id;
    if (id < 0) return false;
    QStringList params;
    params <<  "rm" << QString("%1").arg(id);
    KradClient::anyCommand(params);
    return true;
}
