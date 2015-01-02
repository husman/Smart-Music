#ifndef MIDI_H
#define MIDI_H

#include "global.h"

class Midi {
public:
    midi_file *file;
    Midi(midi_file *file);
    void midi_command_to_ascii(unsigned char *midi_data, unsigned char *midi_ascii_data);
    void wait_for(unsigned int delta_time);
};

#endif