#ifndef BYTE_READER_H
#define BYTE_READER_H

/*
 ============================================================================
 Name        : io.c
 Author      : Haleeq Usman
 Version     : 0.0
 Description : Smart Music Application (dev)
 ============================================================================
 */
#include "global.h"

class ByteReader {
private:
    int BUFFER_MAX_CAPACITY;
    FILE *midi_fp;
    unsigned char *prev_buffer, *buffer;
    int midi_fpos, buffer_len;

public:
    /* Constructors */
    ByteReader(const char *filename);

    int read_byte(unsigned char *dest);
    int unread_byte();
    int read_bytes(unsigned char *dest, int num_bytes);
    int unread_bytes(int num_bytes);

    void deinitialize();
    int initialize(const char *filename);
    unsigned int skip_bytes(unsigned int num_bytes);
};

#endif
