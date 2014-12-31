#ifndef MIDI_H
#define MIDI_H

class Midi {
public:
    static void midi_command_to_ascii(unsigned char *midi_data, unsigned char *midi_ascii_data);
};

#endif