#ifndef MIDI_FILE_DECODER_H
#define MIDI_FILE_DECODER_H

#include "ByteReader.h"

class MidiFileDecoder {
private:
    midi_file *m_file;
    ByteReader *reader;

    bool initialize();

public:
    // Constructors
    MidiFileDecoder(midi_file *file, const char *filename);

    midi_track *get_midi_tracks();
};

#endif