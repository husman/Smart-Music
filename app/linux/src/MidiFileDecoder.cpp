#include "include/MidiFileDecoder.h"

MidiFileDecoder::MidiFileDecoder(midi_file *file) {
    m_file = file;
}

bool MidiFileDecoder::parseFile(const char *filename) {
    unsigned int riff_len, delta_time;
    unsigned short current_track_num;
    unsigned char byte, data[4];
    midi_track *m_tracks;
    ByteReader reader(filename);

    // Parse header
    // RIFF Chunk MThd
    if(reader.read_bytes(data, 4) < 4) {
        printf("Not a midi file\n");
        return false;
    }
    if (memcmp(data, "MThd", 4) != 0) {
        printf("Not a midi file\n");
        return false;
    }
    printf("got: %02x %02x %02x %02x\n",data[0],data[1],data[2],data[3]);

    // RIFF Chunk length
    if(reader.read_bytes(data, 4) < 4) {
        printf("Corrupt midi file 1\n");
        return false;
    }
    printf("got: %02x %02x %02x %02x\n",data[0],data[1],data[2],data[3]);
    riff_len = (unsigned int) ((((((0 | data[0]) << 8) | data[1]) << 8) | data[2]) << 8) | data[3];
    if (riff_len != 6) {
        printf("Corrupt midi file. Expect length of 6 but got %d\n", riff_len);
        return false;
    }

    // Midi format
    if(reader.read_bytes(data, 2) < 2) {
        printf("Corrupt midi file 3\n");
        return false;
    }

    m_file->format = (unsigned short) ((0 | data[0]) << 8) | data[1];
    printf("got [FROMAT]: %02x %02x = %d\n", data[0], data[1], m_file->format);

    // Midi number of tracks
    if(reader.read_bytes(data, 2) < 2) {
        printf("Corrupt midi file 4\n");
        return false;
    }
    m_file->num_tracks = (unsigned short) ((0 | data[0]) << 8) | data[1];
    printf("got: %02x %02x\n", data[0], data[1]);

    // Midi time division
    if(reader.read_bytes(data, 2) < 2) {
        printf("Corrupt midi file 5\n");
        return false;
    }
    m_file->time_division = (unsigned short) ((0 | data[0]) << 8) | data[1];
    printf("got: %02x %02x\n", data[0], data[1]);

    // Allocate tracks
    m_tracks = (midi_track *) malloc(sizeof(midi_track) * m_file->num_tracks);

    byte = 0;
    current_track_num = 0;
    do {
        // Parse track delta time
        delta_time = 0;
        delta_time = delta_time | byte;

        while ((byte & 0x80) == 0x80) {
            if(reader.read_byte(&byte) < 1)
                break;
            delta_time = ((delta_time & 0x7F) << 7) | (byte & 0x7F);
        }


        m_tracks[current_track_num].delta_time = delta_time;
        printf("Parsing Track #%d\n", current_track_num + 1);

        // RIFF Chunk MTrk
        if(reader.read_bytes(data, 4) < 4) {
            printf("Corrupt midi file 6\n");
            return false;
        }
        printf("MTrk: %02x %02x %02x %02x\n", data[0], data[1], data[2], data[3]);
        if (memcmp(data, "MTrk", 4) != 0) {
            printf("No tracks found\n");
            return false;
        }

        // RIFF Chunk length
        if(reader.read_bytes(data, 4) < 4) {
            printf("Corrupt midi file 7\n");
            return false;
        }
        printf("Reading next: %02x %02x %02x %02x bytes for data\n", data[0], data[1], data[2], data[3]);
        riff_len = (unsigned int) (((((((0 | data[0]) << 8) | data[1]) << 8) | data[2]) << 8) | data[3]) - 1;

        m_tracks[current_track_num].size = riff_len;
        m_tracks[current_track_num].track_num = current_track_num + 1;
        m_tracks[current_track_num].data = (unsigned char *) malloc(sizeof(unsigned char)*riff_len);

        if(reader.read_bytes(m_tracks[current_track_num].data, riff_len) < riff_len) {
            printf("Error track terminated before %d bytes.\n", riff_len);
            return false;
        }
        printf("READ: %02x %02x %02x %02x bytes for data\n",
                data[0],
                data[1],
                data[2],
                data[3]
        );
        printf("m_tracks[%d].data (size=%02x) == data!\n",current_track_num,riff_len);
    } while (reader.read_byte(&byte) == 1 && ++current_track_num < m_file->num_tracks);

    m_file->tracks = m_tracks;

    printf("done\n");
    return true;
}