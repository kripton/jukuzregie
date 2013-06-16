#ifndef JACKTHREAD_H
#define JACKTHREAD_H

#include <jack/jack.h>
#include <jack/midiport.h>

#include <QObject>

class JackThread : public QObject {
    Q_OBJECT
public:
    JackThread();
    ~JackThread();

signals:
    void error(QString err);
    void midiEvent(char c0, char c1, char c2);

public slots:
    void setup();

public:
    int process(jack_nframes_t nframes, void *arg);

private:
    jack_client_t *client;
    jack_port_t *output_port;
    jack_port_t *feedback_port;
    jack_port_t *input_port;
};

#endif // JACKTHREAD_H
