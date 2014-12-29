#include "jackthread.h"

// Implemented in main.cpp
int process_wrapper(jack_nframes_t nframes, void *arg);

JackThread::JackThread() :
    QObject()
{
}

JackThread::~JackThread()
{
    if (client != 0)
    {
        jack_client_close(client);
    }
}

void JackThread::setup() {
    if((client = jack_client_open ("RegieControl", JackNullOption, NULL)) == 0)
    {
        emit error("Jack server not running?");
    }
    jack_set_process_callback (client, process_wrapper, 0);
    input_port  = jack_port_register (client, "in",  JACK_DEFAULT_MIDI_TYPE, JackPortIsInput , 0);
    feedback_port = jack_port_register (client, "feedback", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);

    if (jack_activate(client))
    {
        emit error("Cannot activate jack client");
    }

    qDebug() << "Trying to connect ports";
    const char **ports;

    // Connecting our IN port => looking for outputs
    ports = jack_get_ports (client, "nanoKONTROL2", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput);
    if (ports == NULL) {
        qDebug() << "Cannot find any MIDI output ports";
    } else {
        qDebug() << "Trying to connect our" << jack_port_name(input_port) << "to" << ports[0];
        if (jack_connect (client, ports[0], jack_port_name (input_port))) {
            qDebug() << "failed";
        } else {
            qDebug() << "success";
        }
        free (ports);
    }

    // Connecting our feedack (=OUT) port => looking for inputs
    ports = jack_get_ports (client, "nanoKONTROL2", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput);
    if (ports == NULL) {
        qDebug() << "Cannot find any MIDI input ports";
    } else {
        qDebug() << "Trying to connect our" << jack_port_name(feedback_port) << "to" << ports[0];
        if (jack_connect (client, jack_port_name (feedback_port), ports[0])) {
            qDebug() << "failed";
        } else {
            qDebug() << "success";
        }
        free (ports);
    }
}

int JackThread::process(jack_nframes_t nframes, void *arg) {
    Q_UNUSED(arg);
    uint32_t i = 0;
    void* in_port_buf  = jack_port_get_buffer(input_port , nframes);

    jack_midi_event_t in_event;
    jack_nframes_t event_count = jack_midi_get_event_count(in_port_buf);

    //printf ("We have %d events to handle\n", event_count);

    if (!event_count) return 0;

    for (i = 0; i < event_count; i++) {
        jack_midi_event_get(&in_event, in_port_buf, i);

        emit midiEvent((char)in_event.buffer[0], (char)in_event.buffer[1], (char)in_event.buffer[2]);
    }
    return 0;
}

void JackThread::set_led(unsigned char num, unsigned char value)
{
    void* feedback_port_buf = jack_port_get_buffer(feedback_port, 1);
    jack_midi_clear_buffer(feedback_port_buf);
    unsigned char* out;
    out = jack_midi_event_reserve(feedback_port_buf, 0, 3);
    out[0] = 0xb0; // MIDI ControlChange
    out[1] = num;
    out[2] = value;
}
