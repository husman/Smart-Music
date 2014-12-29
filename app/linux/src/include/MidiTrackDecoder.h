#ifndef MIDI_DECODER_H
#define MIDI_DECODER_H

#include "MidiTrackReader.h"

class MidiTrackDecoder {
private:
    midi_file *m_file;
    MidiTrackReader *reader;
    state current_state;

    unsigned char get_next_midi_code();

public:
    // Constructors
    MidiTrackDecoder(midi_file *file, midi_track *track_data);

    state get_state();

    unsigned int get_next_delta_time();

    unsigned char *decode_next_state();

};

#endif