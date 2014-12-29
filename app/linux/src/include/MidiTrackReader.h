#ifndef MIDI_TRACK_READER_H
#define MIDI_TRACK_READER_H

/*
 ============================================================================
 Author      : Haleeq Usman
 Version     : 0.0
 Description : Smart Music Application (dev)
 ============================================================================
 */
#include "global.h"

class MidiTrackReader {
private:
    int BUFFER_MAX_CAPACITY;

    midi_track *m_track;
    unsigned int track_dpos;

public:
    /* Constructors */
    MidiTrackReader(midi_track *track_data);

    int read_track_byte(unsigned char *dest);
    int unread_track_byte();
    int read_track_bytes(unsigned char *dest, int num_bytes);
    int unread_track_bytes(int num_bytes);
};

#endif
