#ifndef MIDI_FILE_DECODER_H
#define MIDI_FILE_DECODER_H

#include "ByteReader.h"

class MidiFileDecoder {
private:
    midi_file *m_file;
    ByteReader *reader;

public:
    // Constructors
    MidiFileDecoder(midi_file *file);

    bool parseFile(const char *filename);
};

#endif