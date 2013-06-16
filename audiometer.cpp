#include "audiometer.h"

AudioMeter::AudioMeter(QWidget *parent) :QSlider(parent) {
    setRange(0, 32768);
    channel = Phonon::AudioDataOutput::LeftChannel;
}

void AudioMeter::dataReceived(const QMap<Phonon::AudioDataOutput::Channel, QVector<qint16> >& data) {
    int i, nSamples, peak = 0;

    nSamples = data[channel].size();

    for(i =0; i < nSamples ;i++){
        if (data[channel][i] > peak) peak = data[channel][i];
    }

    setValue(peak);
}
