#include <stdio.h>
#include "include/Midi.h"

Midi::Midi(midi_file *m_file) {
	file = m_file;
}

void Midi::midi_command_to_ascii(unsigned char *midi_data, unsigned char *midi_ascii_data) {
	char midi_command_format[] = "%02x %02x %02x %02x\r\n";
	snprintf((char *)midi_ascii_data, strlen(midi_command_format), midi_command_format,
			midi_data[0],
			midi_data[1],
			midi_data[2],
			midi_data[3]
	);
}

void Midi::wait_for(unsigned int delta_time) {
	if (delta_time > 0) {
		usleep(delta_time* file->bpm*1000/ file->time_division);
	}
}
