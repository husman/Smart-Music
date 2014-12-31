#include <gtest/gtest.h>

#include "../src/include/Midi.h"

TEST(Midi, midi_command_to_ascii) {
    unsigned char midi_data[4];
    unsigned char midi_ascii_data[100];
    midi_data[0] = 0x90;
    midi_data[1] = 0x4F;
    midi_data[2] = 0x11;
    midi_data[3] = 0x22;

    Midi::midi_command_to_ascii(midi_data, midi_ascii_data);
    ASSERT_STREQ((char *)midi_ascii_data, "90 4f 11 22\r\n");
}
