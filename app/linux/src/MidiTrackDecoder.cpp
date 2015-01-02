#include "include/MidiTrackDecoder.h"

MidiTrackDecoder::MidiTrackDecoder(midi_file *file, midi_track *track_data) {
    m_file = file;
    reader = new MidiTrackReader(track_data);
    current_state = START;
}

state MidiTrackDecoder::get_state() {
    return current_state;
}

unsigned int MidiTrackDecoder::get_next_delta_time() {
    unsigned int delta_time = 0;
    unsigned char byte = 0;

    delta_time = 0;
    if(reader->read_track_byte(&byte) < 1) {
        current_state = DONE;
        exit(-1);
    }
    delta_time = delta_time | byte;

    while ((byte & 0x80) == 0x80) {
        if(reader->read_track_byte(&byte) < 1) {
            current_state = DONE;
            exit(-1);
        }
        delta_time = ((delta_time & 0x7F) << 7) | (byte & 0x7F);
    }


    if(reader->read_track_byte(&byte) < 1) {
        current_state = DONE;
        exit(-1);
    }

    if ((byte & 0x80) == 0x80)
        running_status = 0;
    else
        running_status = 1;

    reader->unread_track_byte();

    return delta_time;
}

unsigned char MidiTrackDecoder::get_next_midi_code() {
    static unsigned char code;

    if (reader->read_track_byte(&code) < 1) {
        current_state = DONE;
        exit(-1);
    }

    return code;
}

unsigned char *MidiTrackDecoder::decode_next_state() {
    unsigned char *midi_data = (unsigned char *)malloc(sizeof(unsigned char )*4);
    unsigned char read_bytes[3];
    static unsigned char code;
    unsigned char read_byte = 0;
    unsigned char *text_data;
    unsigned int i = 0;
    unsigned char midi_ascii_data[100];
    unsigned char time_numerator, time_denominator, metronome_ticks, notes_pqn;
    unsigned int bpm = 0;

    if(!running_status) {
        code = get_next_midi_code();
    }
    switch (code & 0xF0) {
        case NOTE_ON:
            if(reader->read_track_bytes(read_bytes, 2) < 2) {
                current_state = DONE;
            }

            if (read_bytes[1] == 0) {
                //code = 0x80;
                midi_data[0] = 0x08;
                midi_data[1] = 0x80;
            } else {
                midi_data[0] = 0x09;
                midi_data[1] = 0x90;
            }

            midi_data[2] = read_bytes[0];
            midi_data[3] = read_bytes[1];

            printf("Wrote [Note On]: %02x %02x %02x %02x\n",
                    midi_data[0],
                    midi_data[1],
                    midi_data[2],
                    midi_data[3]
            );

            break;
        case NOTE_OFF:
            if(reader->read_track_bytes(read_bytes, 2) < 2) {
                current_state = DONE;
            }

            //code = 0x80;
            midi_data[0] = 0x08;
            midi_data[1] = 0x80;

            midi_data[2] = read_bytes[0];
            midi_data[3] = read_bytes[1];

            printf("Wrote [Note Off]: %02x %02x %02x %02x\n",
                    midi_data[0],
                    midi_data[1],
                    midi_data[2],
                    midi_data[3]
            );

            break;
        case META_SYSTEM:
            // Type
            if(reader->read_track_byte(&read_byte) < 1) {
                current_state = DONE;
            }

            switch (read_byte) {
                case SET_TEMPO:
                    // Length = 3 (always)
                    if(reader->read_track_byte(&read_byte) < 1) {
                        printf("ending1\n");
                        current_state = DONE;
                    }

                    // Tempo data (enforce length = 3)
                    if(reader->read_track_bytes(read_bytes, read_byte) < 3) {
                        printf("ending2\n");
                        current_state = DONE;
                    }
                    bpm = ((((0 | read_bytes[0]) << 8) | read_bytes[1]) << 8) | read_bytes[2];
                    bpm = bpm / 1000;
                    m_file->bpm = bpm;
                    ++i;
                    printf("Detected [TEMPO CHANGE][%d]...\n", i);
                    break;
                case TEXT:
                    // Length = variable
                    if(reader->read_track_byte(&read_byte) < 1) {
                        current_state = DONE;
                    }

                    // Text data (byte length)
                    text_data = (unsigned char *) malloc(sizeof(unsigned char *) * read_byte);
                    if(reader->read_track_bytes(text_data, read_byte) < read_byte) {
                        current_state = DONE;
                    }
                    printf("Read [TEXT]: %s\n", text_data);
                    break;
                case COPYRIGHT:
                    // Length = variable
                    if(reader->read_track_byte(&read_byte) < 1) {
                        current_state = DONE;
                    }

                    // Text data (byte length)
                    text_data = (unsigned char *) malloc(sizeof(unsigned char *) * read_byte);
                    if(reader->read_track_bytes(text_data, read_byte) < read_byte) {
                        current_state = DONE;
                    }
                    printf("Read [COPYRIGHT]: %s\n", text_data);
                    break;
                case TRACK_NAME:
                    // Length = variable
                    if(reader->read_track_byte(&read_byte) < 1) {
                        current_state = DONE;
                    }

                    // Text data (byte length)
                    text_data = (unsigned char *) malloc(sizeof(unsigned char *) * read_byte);
                    if(reader->read_track_bytes(text_data, read_byte) < read_byte) {
                        current_state = DONE;
                    }
                    printf("Read [TRACK NAME]: %s\n", text_data);
                    break;
                case LYRICS:
                    // Length = variable
                    if(reader->read_track_byte(&read_byte) < 1) {
                        current_state = DONE;
                    }

                    // Lyrics data (byte length)
                    text_data = (unsigned char *) malloc(sizeof(unsigned char *) * read_byte);
                    if(reader->read_track_bytes(text_data, read_byte) < read_byte) {
                        current_state = DONE;
                    }
                    printf("Read [LYRICS]: %s\n", text_data);
                    break;
                case MARKER:
                    // Length = variable
                    if(reader->read_track_byte(&read_byte) < 1) {
                        current_state = DONE;
                    }

                    // Marker data (byte length)
                    text_data = (unsigned char *) malloc(sizeof(unsigned char *) * read_byte);
                    if(reader->read_track_bytes(text_data, read_byte) < read_byte) {
                        current_state = DONE;
                    }
                    printf("Read [MARKER]: %s\n", text_data);
                    break;
                case TIME_SIGNATURE:
                    // Length = 4 (always)
                    if(reader->read_track_byte(&read_byte) < 1) {
                        current_state = DONE;
                    }

                    // Time signature data (enforce length = 4)
                    if(reader->read_track_bytes(midi_data, read_byte) < 4) {
                        current_state = DONE;
                    }
                    time_numerator = midi_data[0];
                    m_file->time_signature.numer = time_numerator;

                    time_denominator = 2;
                    for (i = 0; i < midi_data[1]; ++i)
                        time_denominator *= midi_data[1];
                    m_file->time_signature.denom = time_denominator;

                    metronome_ticks = midi_data[2];
                    m_file->time_signature.metro = metronome_ticks;
                    notes_pqn = midi_data[3];
                    m_file->time_signature.notes_pqn = notes_pqn;

                    printf("Read [TIME SIGNATURE]: %02x %02x %02x %02x\n",
                            midi_data[0],
                            midi_data[1],
                            midi_data[2],
                            midi_data[3]
                    );
                    break;
                case SMPTE_OFFSET:
                    // Length = 5 (always)
                    if(reader->read_track_byte(&read_byte) < 1) {
                        current_state = DONE;
                    }

                    // SMPTE offset data (enforce length = 5)
                    if(reader->read_track_bytes(midi_ascii_data, read_byte) < 5) {
                        current_state = DONE;
                    }

                    printf("Read [SMPTE OFFSET]: Time: %d:%d:%d, Fr: %d, SubFr: %d\n",
                            midi_ascii_data[0] & 0x2F,
                            midi_ascii_data[1],
                            midi_ascii_data[2],
                            midi_ascii_data[3],
                            midi_ascii_data[4]
                    );
                    break;
                case KEY_SIGNATURE:
                    // Length = 2 (always)
                    if(reader->read_track_byte(&read_byte) < 1) {
                        current_state = DONE;
                    }

                    // Key signature data (enforce length = 2)
                    if(reader->read_track_bytes(midi_data, read_byte) < 2) {
                        current_state = DONE;
                    }

                    printf("Read [KEY SIGNATURE]: %02x %02x\n",
                            midi_data[0],
                            midi_data[1]
                    );
                    break;
                case END_OF_TRACK:
                    // Length = 0 (always)
                    printf("Read [END OF TRACK]: %02x %2x\n",
                            code,
                            read_byte
                    );
                    current_state = DONE;
                    break;
                default:
                    printf("Read [META_SYSTEM] code not recognized: %02x %02x\n",
                            code,
                            read_byte
                    );

                    // Length = variable
                    if(reader->read_track_byte(&read_byte) < 1) {
                        current_state = DONE;
                    }

                    // Unknown data (byte length)
                    text_data = (unsigned char *) malloc(sizeof(unsigned char) * read_byte);
                    if(reader->read_track_bytes(text_data, read_byte) < read_byte) {
                        current_state = DONE;
                    }
                    printf("Read [META_SYSTEM_UNKNOWN]: %s\n", text_data);
                    break;
            }
            break;
        case CONTROLLER:
            // Read data
            if(reader->read_track_bytes(read_bytes, 2) < 2) {
                current_state = DONE;
            }

            midi_data[0] = 0x0B;
            midi_data[1] = 0xB0;
            midi_data[2] = read_bytes[0];
            midi_data[3] = read_bytes[1];

            printf("Wrote [Controller]: %02x %02x %02x %02x\n",
                    midi_data[0],
                    midi_data[1],
                    midi_data[2],
                    midi_data[3]
            );
            break;
        case POLY_AFTERTOUCH:
            // Read data
            if(reader->read_track_bytes(read_bytes, 2) < 2) {
                current_state = DONE;
            }

            midi_data[0] = 0x0A;
            midi_data[1] = 0xA0;
            midi_data[2] = read_bytes[0]; // Note number
            midi_data[3] = read_bytes[1]; // Pressure

            printf("Wrote [Poly Aftertouch]: %02x %02x %02x %02x\n",
                    midi_data[0],
                    midi_data[1],
                    midi_data[2],
                    midi_data[3]
            );
            break;
        case PROGRAM_CHANGE:
            // Read data
            if(reader->read_track_byte(&read_byte) < 1) {
                current_state = DONE;
            }

            midi_data[0] = 0x0C;
            midi_data[1] = 0xC0;
            midi_data[2] = read_byte; // Program number
            midi_data[3] = 0;

            printf("Wrote [Program Change]: %02x %02x %02x %02x\n",
                    midi_data[0],
                    midi_data[1],
                    midi_data[2],
                    midi_data[3]
            );

            break;
        default:
            printf("Read [CODE] code not recognized: %02x\n", code);

            // Length = variable
            if(reader->read_track_byte(&read_byte) < 1) {
                current_state = DONE;
            }

            // unknown data (byte length)
            text_data = (unsigned char *) malloc(sizeof(unsigned char) * read_byte);
            if(reader->read_track_bytes(text_data, read_byte) < read_byte) {
                current_state = DONE;
            }
            printf("Read [UNKNOWN]: %s\n", text_data);
            break;
    }

    return midi_data;
}