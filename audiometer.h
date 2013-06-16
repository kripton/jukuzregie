#ifndef AUDIOMETER_H
#define AUDIOMETER_H

#include <QSlider>
#include <phonon/AudioDataOutput>

// Reference: https://gist.github.com/saidinesh5/3692753

class AudioMeter : public QSlider
{
    Q_OBJECT

public:
    explicit AudioMeter(QWidget *parent = 0);
    Phonon::AudioDataOutput::Channel channel;

public slots:
    void dataReceived(const QMap<Phonon::AudioDataOutput::Channel, QVector<qint16> >& data);

};

#endif // AUDIOMETER_H
