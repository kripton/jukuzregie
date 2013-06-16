#include "kradclient.h"

KradClient::KradClient(QObject *parent) :
    QObject(parent)
{
}

void KradClient::anyCommand(QStringList params)
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("XDG_RUNTIME_DIR", QString("%1/streaming/xdg").arg(QDir::homePath()));
    QProcess* process = new QProcess();
    process->setProcessEnvironment(env);
    params.prepend("jukuz");
    qDebug() << params;
    process->start("/usr/bin/krad_radio", params);
    process->waitForFinished();
    process->~QProcess();
    process = NULL;
}

void KradClient::launch()
{
    KradClient::kill();
    KradClient::anyCommand(QStringList() << "launch");
    QProcess* kProcess = new QProcess();
    kProcess->start("sleep", QStringList() << "1");
    kProcess->waitForFinished();
}

void KradClient::kill()
{
    QProcess* kProcess = new QProcess();
    kProcess->start("killall", QStringList() << "-9" << "krad_radio_daemon");
    kProcess->waitForFinished();
}

qint16 KradClient::playStream(QUrl streamUrl) {
    qint16 retval = -1;
    bool parseOkay = false;

    // Play the stream
    anyCommand(QStringList() << "play" << streamUrl.host() << QString("%1").arg(streamUrl.port()) << streamUrl.path());

    QProcess* process = new QProcess();
    process->start("/usr/bin/krad_radio", QStringList() << "jukuz" << "ls");
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

qint16 KradClient::getRecordId()
{
    int retval = -1;
    bool parseOkay = false;

    QProcess* process = new QProcess();
    process->start("/usr/bin/krad_radio", QStringList() << "jukuz" << "ls");
    process->waitForFinished();
    foreach (QString line, process->readAll().split('\n')) {
        if (!line.contains("record")) continue;
        retval = line.split(':')[1].trimmed().split(' ')[0].toInt(&parseOkay);
    }
    if (!parseOkay) retval = -1;
    process->~QProcess();
    process = NULL;

    return retval;
}

qint16 KradClient::getTransmitId()
{
    int retval = -1;
    bool parseOkay = false;

    QProcess* process = new QProcess();
    process->start("/usr/bin/krad_radio", QStringList() << "jukuz" << "ls");
    process->waitForFinished();
    foreach (QString line, process->readAll().split('\n')) {
        if (!line.contains("transmit")) continue;
        retval = line.split(':')[1].trimmed().split(' ')[0].toInt(&parseOkay);
    }
    if (!parseOkay) retval = -1;
    process->~QProcess();
    process = NULL;

    return retval;
}

void KradClient::ls()
{
    QProcess* process = new QProcess();
    process->start("/usr/bin/krad_radio", QStringList() << "jukuz" << "ls");
    process->waitForFinished();
    qDebug() << "KRAD LS:" <<  process->readAll();

    process->~QProcess();
    process = NULL;
}
