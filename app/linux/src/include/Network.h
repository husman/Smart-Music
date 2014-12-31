#ifndef NETWORK_H
#define NETWORK_H

#include "Midi.h"

class Network {
private:
    Midi *midi;

    typedef struct midi_track_thread_args {
        Network *instance;
        unsigned int track_num;
    } midi_track_thread_args;

public:
    Network(Midi *midi_info);

    static void *play_midi_track_udp_ts(void *thread_args);

    void send_udp_packet(unsigned char *midi_ascii_data);
    void playMidi_udp();
    void hold_pedal(bool down_state);
    void *play_midi_track_udp(int track_num);
};

#endif