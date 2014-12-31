#include <stdio.h>
#include <GSS/GSS.h>
#include "include/Midi.h"

void Midi::midi_command_to_ascii(unsigned char *midi_data, unsigned char *midi_ascii_data) {
	char midi_command_format[] = "%02x %02x %02x %02x\r\n";
	snprintf((char *)midi_ascii_data, strlen(midi_command_format), midi_command_format,
			midi_data[0],
			midi_data[1],
			midi_data[2],
			midi_data[3]
	);
}