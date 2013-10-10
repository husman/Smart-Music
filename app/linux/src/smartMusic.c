/*
 ============================================================================
 Name        : smartMusic.c
 Author      : Haleeq Usman
 Version     : 0.0
 Description : Smart Music Application (dev)
 ============================================================================
 */
#include <time.h>
#include "include/global.h"

void delay(double ms) {
	clock_t start_t;

	start_t = clock();
	while ((double) (clock() - start_t) / (CLOCKS_PER_SEC / 1000) < ms)
		;
}

void playMidi2(FILE *drv_ptr, int track_num) {
	FILE *drv_ptr2 = NULL;
	drv_ptr2 = fopen("/dev/skel1", "wb");
	if(drv_ptr2 == NULL) {
		printf("device #2 not found.\n");
		return;
	}
	state current_state = START;

	unsigned char code, bytes[3], byte = 0, running_status;
	unsigned char *variable_bytes;
	unsigned int riff_len, i = 0;
	unsigned int bpm, delta_time = 0;
	unsigned char data[4], data2[5];
	unsigned short format, num_tracks, time_division;
	unsigned char time_numerator, time_denominator, metronome_ticks, notes_pqn;
	char midi_key_signature, midi_scale;
	int current_track_num = 0;
	unsigned char *variable_data;

	// Parse header
	// RIFF Chunk MThd
	if (read_bytes(data, 4) < 4) {
		printf("Not a midi file\n");
		return;
	}
	if (memcmp(data, "MThd", 4) != 0) {
		printf("Not a midi file\n");
		return;
	}

	// RIFF Chunk length
	if (read_bytes(data, 4) < 4) {
		printf("Corrupt midi file\n");
		return;
	}
	riff_len = ((((((0 | data[0]) << 8) | data[1]) << 8) | data[2]) << 8)
			| data[3];
	if (riff_len != 6) {
		printf("Corrupt midi file\n");
		return;
	}

	// Midi format
	if (read_bytes(data, 2) < 2) {
		printf("Corrupt midi file\n");
		return;
	}
	format = ((0 | data[0]) << 8) | data[1];

	// Midi number of tracks
	if (read_bytes(data, 2) < 2) {
		printf("Corrupt midi file\n");
		return;
	}
	num_tracks = ((0 | data[0]) << 8) | data[1];

	// Midi time division
	if (read_bytes(data, 2) < 2) {
		printf("Corrupt midi file\n");
		return;
	}
	time_division = ((0 | data[0]) << 8) | data[1];

	do {
		current_state = START;
		printf("Parsing Track #%d\n", ++current_track_num);
		//delay(1000);
		// Parse track delta time
		delta_time = 0;
		delta_time = delta_time | byte;

		while ((byte & 0x80) == 0x80) {
			if (read_byte(&byte) < 1) {
				current_state = DONE;
				break;
			}
			delta_time = ((delta_time & 0x7F) << 7) | (byte & 0x7F);
		}

		printf("delta time = %02x\n", byte);

		// RIFF Chunk MTrk
		if (read_bytes(data, 4) < 4) {
			printf("Corrupt midi file\n");
			return;
		}
		printf("data time = %02x %02x %02x %02x\n", data[0], data[1], data[2],
				data[3]);
		if (memcmp(data, "MTrk", 4) != 0) {
			printf("No tracks found\n");
			return;
		}

		// RIFF Chunk length
		if (read_bytes(data, 4) < 4) {
			printf("Corrupt midi file\n");
			return;
		}

		riff_len = (((((((0 | data[0]) << 8) | data[1]) << 8) | data[2]) << 8)
				| data[3]) - 1;
		if (track_num != current_track_num && current_track_num != 1) {
			if (skip_bytes(riff_len) < riff_len) {
				printf("Error skipping %d bytes\n", riff_len);
				return;
			}

			current_state = DONE;
			printf("skipping %02x bytes\n", riff_len);
		}

		running_status = 0;
		while (current_state != DONE) {
			delta_time = 0;
			if (read_byte(&byte) < 1) {
				current_state = DONE;
				break;
			}
			delta_time = delta_time | byte;

			while ((byte & 0x80) == 0x80) {
				if (read_byte(&byte) < 1) {
					current_state = DONE;
					break;
				}
				delta_time = ((delta_time & 0x7F) << 7) | (byte & 0x7F);
			}

			//printf("next delta = %02x\n", delta_time);

			if (read_byte(&byte) < 1) {
				current_state = DONE;
				break;
			}

			if ((byte & 0x80) == 0x80) {
				running_status = 0;
			} else {
				running_status = 1;
			}

			unread_byte();

			if (!running_status)
				if (read_byte(&code) < 1) {
					current_state = DONE;
					break;
				}

			switch (code & 0xF0) {
			case NOTE_ON:
				if (read_bytes(bytes, 2) < 2) {
					current_state = DONE;
					continue;
				}

				if (bytes[1] == 0) {
					//code = 0x80;
					data[0] = 0x08;
					data[1] = 0x80;
				} else {
					data[0] = 0x09;
					data[1] = 0x90;
				}

				data[2] = bytes[0];
				data[3] = bytes[1];

				fwrite(data, 4, 1, drv_ptr);
				fwrite(data, 4, 1, drv_ptr2);
				fflush(drv_ptr);
				fflush(drv_ptr2);
				printf("Wrote [Note On]: %02x %02x %02x %02x\n", data[0],
						data[1], data[2], data[3]);
				break;
			case NOTE_OFF:
				if (read_bytes(bytes, 2) < 2) {
					current_state = DONE;
					continue;
				}

				//code = 0x80;
				data[0] = 0x08;
				data[1] = 0x80;

				data[2] = bytes[0];
				data[3] = bytes[1];

				fwrite(data, 4, 1, drv_ptr);
				fwrite(data, 4, 1, drv_ptr2);
				fflush(drv_ptr);
				fflush(drv_ptr2);
				printf("Wrote [Note Off]: %02x %02x %02x %02x\n", data[0],
						data[1], data[2], data[3]);
				break;
			case META_SYSTEM:
				// Type
				if (read_byte(&byte) < 1) {
					current_state = DONE;
					continue;
				}

				switch (byte) {
				case SET_TEMPO:
					// Length = 3 (always)
					if (read_byte(&byte) < 1) {
						printf("ending1\n");
						current_state = DONE;
						continue;
					}

					// Tempo data (enforce length = 3)
					if (read_bytes(bytes, byte) < 3) {
						printf("ending2\n");
						current_state = DONE;
						continue;
					}
					bpm = ((((0 | bytes[0]) << 8) | bytes[1]) << 8) | bytes[2];
					bpm = bpm / 1000;

					++i;
					printf("Found [%d] tempo commands\n", i);
					break;
				case TEXT:
					// Length = variable
					if (read_byte(&byte) < 1) {
						current_state = DONE;
						continue;
					}

					// Text data (byte length)
					variable_bytes = (unsigned char *) malloc(
							sizeof(unsigned char *) * byte);
					if (read_bytes(variable_bytes, byte) < byte) {
						current_state = DONE;
						continue;
					}
					printf("Read [TEXT]: %s\n", variable_bytes);
					break;
				case COPYRIGHT:
					// Length = variable
					if (read_byte(&byte) < 1) {
						current_state = DONE;
						continue;
					}

					// Text data (byte length)
					variable_bytes = (unsigned char *) malloc(
							sizeof(unsigned char *) * byte);
					if (read_bytes(variable_bytes, byte) < byte) {
						current_state = DONE;
						continue;
					}
					printf("Read [COPYRIGHT]: %s\n", variable_bytes);
					break;
				case TRACK_NAME:
					// Length = variable
					if (read_byte(&byte) < 1) {
						current_state = DONE;
						continue;
					}

					// Text data (byte length)
					variable_bytes = (unsigned char *) malloc(
							sizeof(unsigned char *) * byte);
					if (read_bytes(variable_bytes, byte) < byte) {
						current_state = DONE;
						continue;
					}
					printf("Read [TRACK NAME]: %s\n", variable_bytes);
					break;
				case LYRICS:
					// Length = variable
					if (read_byte(&byte) < 1) {
						current_state = DONE;
						continue;
					}

					// Lyrics data (byte length)
					variable_bytes = (unsigned char *) malloc(
							sizeof(unsigned char *) * byte);
					if (read_bytes(variable_bytes, byte) < byte) {
						current_state = DONE;
						continue;
					}
					printf("Read [LYRICS]: %s\n", variable_bytes);
					break;
				case MARKER:
					// Length = variable
					if (read_byte(&byte) < 1) {
						current_state = DONE;
						continue;
					}

					// Marker data (byte length)
					variable_bytes = (unsigned char *) malloc(
							sizeof(unsigned char *) * byte);
					if (read_bytes(variable_bytes, byte) < byte) {
						current_state = DONE;
						continue;
					}
					printf("Read [MARKER]: %s\n", variable_bytes);
					break;
				case TIME_SIGNATURE:
					// Length = 4 (always)
					if (read_byte(&byte) < 1) {
						current_state = DONE;
						continue;
					}

					// Time signature data (enforce length = 4)
					if (read_bytes(data, byte) < 4) {
						current_state = DONE;
						continue;
					}
					time_numerator = data[0];

					time_denominator = 2;
					for (i = 0; i < data[1]; ++i)
						time_denominator *= data[1];

					metronome_ticks = data[2];
					notes_pqn = data[3];

					printf("Read [TIME SIGNATURE]: %02x %02x %02x %02x\n",
							data[0], data[1], data[2], data[3]);
					break;
				case SMPTE_OFFSET:
					// Length = 5 (always)
					if (read_byte(&byte) < 1) {
						current_state = DONE;
						continue;
					}

					// SMPTE offset data (enforce length = 5)
					if (read_bytes(data2, byte) < 5) {
						current_state = DONE;
						continue;
					}

					printf(
							"Read [SMPTE OFFSET]: Time: %d:%d:%d, Fr: %d, SubFr: %d\n",
							data2[0] & 0x2F, data2[1], data2[2], data2[3],
							data2[4]);
					break;
				case KEY_SIGNATURE:
					// Length = 2 (always)
					if (read_byte(&byte) < 1) {
						current_state = DONE;
						continue;
					}

					// Key signature data (enforce length = 2)
					if (read_bytes(data, byte) < 2) {
						current_state = DONE;
						continue;
					}
					midi_key_signature = data[0];
					midi_scale = data[1];

					printf("Read [KEY SIGNATURE]: %02x %02x\n", data[0],
							data[1]);
					break;
				case END_OF_TRACK:
					// Length = 0 (always)
					printf("Read [END OF TRACK]: %02x %2x\n", code, byte);
					current_state = DONE;
					break;
				default:
					printf(
							"Read [META_SYSTEM] code not recognized: %02x %02x\n",
							code, byte);
					return;
					break;
				}
				break;
			case CONTROLLER:
				// Read data
				if (read_bytes(bytes, 2) < 2) {
					current_state = DONE;
					continue;
				}

				data2[0] = 0x0B;
				data2[1] = 0xB0;
				data2[2] = bytes[0];
				data2[3] = bytes[1];

				fwrite(data2, 4, 1, drv_ptr);
				fwrite(data2, 4, 1, drv_ptr2);
				fflush(drv_ptr);
				fflush(drv_ptr2);
				printf("Wrote [Controller]: %02x %02x %02x %02x\n", data2[0],
						data2[1], data2[2], data2[3]);
				break;
			case POLY_AFTERTOUCH:
				// Read data
				if (read_bytes(bytes, 2) < 2) {
					current_state = DONE;
					continue;
				}

				data[0] = 0x0A;
				data[1] = 0xA0;
				data[2] = bytes[0]; // Note number
				data[3] = bytes[1]; // Pressure

				fwrite(data, 4, 1, drv_ptr);
				fwrite(data, 4, 1, drv_ptr2);
				fflush(drv_ptr);
				fflush(drv_ptr2);
				printf("Wrote [Poly Aftertouch]: %02x %02x %02x %02x\n",
						data[0], data[1], data[2], data[3]);
				break;
			case PROGRAM_CHANGE:
				// Read data
				if (read_byte(&byte) < 1) {
					current_state = DONE;
					continue;
				}

				data[0] = 0x0C;
				data[1] = 0xC0;
				data[2] = byte; // Program number

				fwrite(data, 3, 1, drv_ptr);
				fwrite(data, 3, 1, drv_ptr2);
				fflush(drv_ptr);
				fflush(drv_ptr2);
				printf("Wrote [Program Change]: %02x %02x %02x\n", data[0],
						data[1], data[2]);

				break;
			default:
				printf("Read [CODE] code not recognized: %02x\n", code);
				return;
				break;
			}
			if (delta_time > 0) {
				//printf("delaying: %d ms\n",delta_time*bpm/time_division);
				delay(delta_time * bpm / time_division);
			}
		}

		printf("Found [%d] tempo commands\n", i);
	} while (read_byte(&byte) > 0 && current_track_num < num_tracks);

}

midi_file *m_file2;

void *play_midi_track(void *track_data)
{
     midi_track *m_track;
     m_track = (midi_track *)track_data;
     printf("Playing track #%d | size = %02x\n", m_track->track_num, m_track->size);

     printf("TIME SIGNATURE NPQ = %02x DIV = %02x BPM = %02x\n",
    		 m_file2->time_signature.notes_pqn, m_file2->time_division, m_file2->bpm);

     FILE *drv_ptr = m_file2->drv_ptr;

     /* Start track playback */
     state current_state = START;

	unsigned char code, bytes[3], byte = 0, running_status;
	unsigned char *variable_bytes;
	unsigned int riff_len, i = 0, track_dpos = 0;
	unsigned int bpm, delta_time = 0;
	unsigned char data[4], data2[5], data3[10];
	unsigned short format, num_tracks, time_division;
	unsigned char time_numerator, time_denominator, metronome_ticks, notes_pqn;
	char midi_key_signature, midi_scale;
	int current_track_num = 0;
	unsigned char *variable_data;
	int skip_delta_time = 0;


	do {
		current_state = START;

		printf("delta time = %02x\n", m_track->delta_time);
		if (m_track->delta_time > 0) {
			//printf("delaying: %d ms\n",m_track->delta_time*m_file2->bpm/m_file2->time_division);
			usleep(m_track->delta_time*m_file2->bpm*1000/m_file2->time_division);
		}

		running_status = 0;
		while (current_state != DONE) {
			if(!skip_delta_time) {
				delta_time = 0;
				if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
					current_state = DONE;
					break;
				}
				delta_time = delta_time | byte;

				while ((byte & 0x80) == 0x80) {
					if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
						current_state = DONE;
						break;
					}
					delta_time = ((delta_time & 0x7F) << 7) | (byte & 0x7F);
				}

				printf("delta_time = %02x\n",delta_time);


				if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
					current_state = DONE;
					break;
				}
				printf("read code = %02x\n",byte);

				if ((byte & 0x80) == 0x80)
					running_status = 0;
				else
					running_status = 1;

				unread_track_byte(&track_dpos, m_track);
			} else {
				delta_time = m_track->delta_time;
				skip_delta_time = 0;
			}

			if (!running_status)
				if (read_track_byte(&track_dpos, m_track, &code) < 1) {
					current_state = DONE;
					break;
				}


			switch (code & 0xF0) {
			case NOTE_ON:
				if (read_track_bytes(&track_dpos, m_track, bytes, 2) < 2) {
					current_state = DONE;
					continue;
				}

				if (bytes[1] == 0) {
					//code = 0x80;
					data[0] = 0x08;
					data[1] = 0x80;
				} else {
					data[0] = 0x09;
					data[1] = 0x90;
				}

				data[2] = bytes[0];
				data[3] = bytes[1];

				fwrite(data, 4, 1, drv_ptr);
				fflush(drv_ptr);
				printf("Wrote [Note On]: %02x %02x %02x %02x\n", data[0],
						data[1], data[2], data[3]);
				break;
			case NOTE_OFF:
				if (read_track_bytes(&track_dpos, m_track, bytes, 2) < 2) {
					current_state = DONE;
					continue;
				}

				//code = 0x80;
				data[0] = 0x08;
				data[1] = 0x80;

				data[2] = bytes[0];
				data[3] = bytes[1];

				fwrite(data, 4, 1, drv_ptr);
				fflush(drv_ptr);
				printf("Wrote [Note Off]: %02x %02x %02x %02x\n", data[0],
						data[1], data[2], data[3]);
				break;
			case META_SYSTEM:
				// Type
				if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
					current_state = DONE;
					continue;
				}

				switch (byte) {
				case SET_TEMPO:
					// Length = 3 (always)
					if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
						printf("ending1\n");
						current_state = DONE;
						continue;
					}

					// Tempo data (enforce length = 3)
					if (read_track_bytes(&track_dpos, m_track, bytes, byte) < 3) {
						printf("ending2\n");
						current_state = DONE;
						continue;
					}
					bpm = ((((0 | bytes[0]) << 8) | bytes[1]) << 8) | bytes[2];
					bpm = bpm / 1000;
					m_file2->bpm = bpm;
					++i;
					printf("Detected [TEMPO CHANGE][%d]...\n", i);
					break;
				case TEXT:
					// Length = variable
					if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
						current_state = DONE;
						continue;
					}

					// Text data (byte length)
					variable_bytes = (unsigned char *) malloc(
							sizeof(unsigned char *) * byte);
					if (read_track_bytes(&track_dpos, m_track, variable_bytes, byte) < byte) {
						current_state = DONE;
						continue;
					}
					printf("Read [TEXT]: %s\n", variable_bytes);
					break;
				case COPYRIGHT:
					// Length = variable
					if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
						current_state = DONE;
						continue;
					}

					// Text data (byte length)
					variable_bytes = (unsigned char *) malloc(
							sizeof(unsigned char *) * byte);
					if (read_track_bytes(&track_dpos, m_track, variable_bytes, byte) < byte) {
						current_state = DONE;
						continue;
					}
					printf("Read [COPYRIGHT]: %s\n", variable_bytes);
					break;
				case TRACK_NAME:
					// Length = variable
					if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
						current_state = DONE;
						continue;
					}

					// Text data (byte length)
					variable_bytes = (unsigned char *) malloc(
							sizeof(unsigned char *) * byte);
					if (read_track_bytes(&track_dpos, m_track, variable_bytes, byte) < byte) {
						current_state = DONE;
						continue;
					}
					printf("Read [TRACK NAME]: %s\n", variable_bytes);
					break;
				case LYRICS:
					// Length = variable
					if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
						current_state = DONE;
						continue;
					}

					// Lyrics data (byte length)
					variable_bytes = (unsigned char *) malloc(
							sizeof(unsigned char *) * byte);
					if (read_track_bytes(&track_dpos, m_track, variable_bytes, byte) < byte) {
						current_state = DONE;
						continue;
					}
					printf("Read [LYRICS]: %s\n", variable_bytes);
					break;
				case MARKER:
					// Length = variable
					if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
						current_state = DONE;
						continue;
					}

					// Marker data (byte length)
					variable_bytes = (unsigned char *) malloc(
							sizeof(unsigned char *) * byte);
					if (read_track_bytes(&track_dpos, m_track, variable_bytes, byte) < byte) {
						current_state = DONE;
						continue;
					}
					printf("Read [MARKER]: %s\n", variable_bytes);
					break;
				case TIME_SIGNATURE:
					// Length = 4 (always)
					if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
						current_state = DONE;
						continue;
					}

					// Time signature data (enforce length = 4)
					if (read_track_bytes(&track_dpos, m_track, data, byte) < 4) {
						current_state = DONE;
						continue;
					}
					time_numerator = data[0];
					m_file2->time_signature.numer = time_numerator;

					time_denominator = 2;
					for (i = 0; i < data[1]; ++i)
						time_denominator *= data[1];
					m_file2->time_signature.denom = time_denominator;

					metronome_ticks = data[2];
					m_file2->time_signature.metro = metronome_ticks;
					notes_pqn = data[3];
					m_file2->time_signature.notes_pqn = notes_pqn;

					printf("Read [TIME SIGNATURE]: %02x %02x %02x %02x\n",
							data[0], data[1], data[2], data[3]);
					break;
				case SMPTE_OFFSET:
					// Length = 5 (always)
					if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
						current_state = DONE;
						continue;
					}

					// SMPTE offset data (enforce length = 5)
					if (read_track_bytes(&track_dpos, m_track, data2, byte) < 5) {
						current_state = DONE;
						continue;
					}

					printf(
							"Read [SMPTE OFFSET]: Time: %d:%d:%d, Fr: %d, SubFr: %d\n",
							data2[0] & 0x2F, data2[1], data2[2], data2[3],
							data2[4]);
					break;
				case KEY_SIGNATURE:
					// Length = 2 (always)
					if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
						current_state = DONE;
						continue;
					}

					// Key signature data (enforce length = 2)
					if (read_track_bytes(&track_dpos, m_track, data, byte) < 2) {
						current_state = DONE;
						continue;
					}
					midi_key_signature = data[0];
					midi_scale = data[1];

					printf("Read [KEY SIGNATURE]: %02x %02x\n", data[0],
							data[1]);
					break;
				case END_OF_TRACK:
					// Length = 0 (always)
					printf("Read [END OF TRACK]: %02x %2x\n", code, byte);
					current_state = DONE;
					break;
				default:
					printf("Read [META_SYSTEM] code not recognized: %02x %02x\n", code, byte);

					// Length = variable
					if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
						current_state = DONE;
						continue;
					}

					// Unknown data (byte length)
					variable_bytes = (unsigned char *) malloc(sizeof(unsigned char) * byte);
					if (read_track_bytes(&track_dpos, m_track, variable_bytes, byte) < byte) {
						current_state = DONE;
						continue;
					}
					printf("Read [META_SYSTEM_UNKNOWN]: %s\n", variable_bytes);

					break;
				}
				break;
			case CONTROLLER:
				// Read data
				if (read_track_bytes(&track_dpos, m_track, bytes, 2) < 2) {
					current_state = DONE;
					continue;
				}

				data2[0] = 0x0B;
				data2[1] = 0xB0;
				data2[2] = bytes[0];
				data2[3] = bytes[1];

				fwrite(data2, 4, 1, drv_ptr);
				fflush(drv_ptr);
				printf("Wrote [Controller]: %02x %02x %02x %02x\n", data2[0],
						data2[1], data2[2], data2[3]);
				break;
			case POLY_AFTERTOUCH:
				// Read data
				if (read_track_bytes(&track_dpos, m_track, bytes, 2) < 2) {
					current_state = DONE;
					continue;
				}

				data[0] = 0x0A;
				data[1] = 0xA0;
				data[2] = bytes[0]; // Note number
				data[3] = bytes[1]; // Pressure

				fwrite(data, 4, 1, drv_ptr);
				fflush(drv_ptr);
				printf("Wrote [Poly Aftertouch]: %02x %02x %02x %02x\n",
						data[0], data[1], data[2], data[3]);
				break;
			case PROGRAM_CHANGE:
				// Read data
				if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
					current_state = DONE;
					continue;
				}

				data[0] = 0x0C;
				data[1] = 0xC0;
				data[2] = byte; // Program number

				fwrite(data, 3, 1, drv_ptr);
				fflush(drv_ptr);
				printf("Wrote [Program Change]: %02x %02x %02x\n", data[0],
						data[1], data[2]);

				break;
			default:
				printf("Read [CODE] code not recognized: %02x\n", code);

				// Length = variable
				if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
					current_state = DONE;
					continue;
				}

				// unknown data (byte length)
				variable_bytes = (unsigned char *) malloc(sizeof(unsigned char) * byte);
				if (read_track_bytes(&track_dpos, m_track, variable_bytes, byte) < byte) {
					current_state = DONE;
					continue;
				}
				printf("Read [UNKNOWN]: %s\n", variable_bytes);

				break;
			}
			if (delta_time > 0) {
				usleep(delta_time*m_file2->bpm*1000/m_file2->time_division);
			}
		}

		printf("Found [%d] tempo commands\n", i);
	} while (read_track_byte(&track_dpos, m_track, &byte) > 0 && current_track_num < num_tracks);
}

void *play_midi_track_net(void *track_data)
{
     midi_track *m_track;
     m_track = (midi_track *)track_data;
     printf("Playing track #%d | size = %02x\n", m_track->track_num, m_track->size);

     printf("TIME SIGNATURE NPQ = %02x DIV = %02x BPM = %02x\n",
    		 m_file2->time_signature.notes_pqn, m_file2->time_division, m_file2->bpm);


     /* Start track playback */
     state current_state = START;

	unsigned char code, bytes[3], byte = 0, running_status;
	unsigned char *variable_bytes;
	unsigned int riff_len, i = 0, track_dpos = 0;
	unsigned int bpm, delta_time = 0;
	unsigned char data[100], data2[100], data3[100];
	unsigned short format, num_tracks, time_division;
	unsigned char time_numerator, time_denominator, metronome_ticks, notes_pqn;
	char midi_key_signature, midi_scale;
	int current_track_num = 0;
	unsigned char *variable_data;
	int skip_delta_time = 0;

	do {
		current_state = START;

		printf("delta time = %02x\n", m_track->delta_time);
		if (m_track->delta_time > 0) {
			//printf("delaying: %d ms\n",m_track->delta_time*m_file2->bpm/m_file2->time_division);
			usleep(m_track->delta_time*m_file2->bpm*1000/m_file2->time_division);
		}

		running_status = 0;
		while (current_state != DONE) {
			if(!skip_delta_time) {
				delta_time = 0;
				if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
					current_state = DONE;
					break;
				}
				delta_time = delta_time | byte;

				while ((byte & 0x80) == 0x80) {
					if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
						current_state = DONE;
						break;
					}
					delta_time = ((delta_time & 0x7F) << 7) | (byte & 0x7F);
				}

				printf("delta_time = %02x\n",delta_time);


				if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
					current_state = DONE;
					break;
				}
				printf("read code = %02x\n",byte);

				if ((byte & 0x80) == 0x80)
					running_status = 0;
				else
					running_status = 1;

				unread_track_byte(&track_dpos, m_track);
			} else {
				delta_time = m_track->delta_time;
				skip_delta_time = 0;
			}

			if (!running_status)
				if (read_track_byte(&track_dpos, m_track, &code) < 1) {
					current_state = DONE;
					break;
				}


			switch (code & 0xF0) {
			case NOTE_ON:
				if (read_track_bytes(&track_dpos, m_track, bytes, 2) < 2) {
					current_state = DONE;
					continue;
				}

				if (bytes[1] == 0) {
					//code = 0x80;
					data[0] = 0x08;
					data[1] = 0x80;
				} else {
					data[0] = 0x09;
					data[1] = 0x90;
				}

				data[2] = bytes[0];
				data[3] = bytes[1];


				snprintf(data, sizeof(data3), "%02x %02x %02x %02x\r\n",
						 data[0],data[1],data[2],data[3]);
				write(m_file2->connfd, data, strlen(data));

				printf("Wrote [Note On]: %02x %02x %02x %02x\n", data[0],
						data[1], data[2], data[3]);
				break;
			case NOTE_OFF:
				if (read_track_bytes(&track_dpos, m_track, bytes, 2) < 2) {
					current_state = DONE;
					continue;
				}

				//code = 0x80;
				data[0] = 0x08;
				data[1] = 0x80;

				data[2] = bytes[0];
				data[3] = bytes[1];


				snprintf(data, sizeof(data3), "%02x %02x %02x %02x\r\n",
						 data[0],data[1],data[2],data[3]);
				write(m_file2->connfd, data, strlen(data));
				printf("Wrote [Note Off]: %02x %02x %02x %02x\n", data[0],
						data[1], data[2], data[3]);
				break;
			case META_SYSTEM:
				// Type
				if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
					current_state = DONE;
					continue;
				}

				switch (byte) {
				case SET_TEMPO:
					// Length = 3 (always)
					if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
						printf("ending1\n");
						current_state = DONE;
						continue;
					}

					// Tempo data (enforce length = 3)
					if (read_track_bytes(&track_dpos, m_track, bytes, byte) < 3) {
						printf("ending2\n");
						current_state = DONE;
						continue;
					}
					bpm = ((((0 | bytes[0]) << 8) | bytes[1]) << 8) | bytes[2];
					bpm = bpm / 1000;
					m_file2->bpm = bpm;
					++i;
					printf("Detected [TEMPO CHANGE][%d]...\n", i);
					break;
				case TEXT:
					// Length = variable
					if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
						current_state = DONE;
						continue;
					}

					// Text data (byte length)
					variable_bytes = (unsigned char *) malloc(
							sizeof(unsigned char *) * byte);
					if (read_track_bytes(&track_dpos, m_track, variable_bytes, byte) < byte) {
						current_state = DONE;
						continue;
					}
					printf("Read [TEXT]: %s\n", variable_bytes);
					break;
				case COPYRIGHT:
					// Length = variable
					if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
						current_state = DONE;
						continue;
					}

					// Text data (byte length)
					variable_bytes = (unsigned char *) malloc(
							sizeof(unsigned char *) * byte);
					if (read_track_bytes(&track_dpos, m_track, variable_bytes, byte) < byte) {
						current_state = DONE;
						continue;
					}
					printf("Read [COPYRIGHT]: %s\n", variable_bytes);
					break;
				case TRACK_NAME:
					// Length = variable
					if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
						current_state = DONE;
						continue;
					}

					// Text data (byte length)
					variable_bytes = (unsigned char *) malloc(
							sizeof(unsigned char *) * byte);
					if (read_track_bytes(&track_dpos, m_track, variable_bytes, byte) < byte) {
						current_state = DONE;
						continue;
					}
					printf("Read [TRACK NAME]: %s\n", variable_bytes);
					break;
				case LYRICS:
					// Length = variable
					if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
						current_state = DONE;
						continue;
					}

					// Lyrics data (byte length)
					variable_bytes = (unsigned char *) malloc(
							sizeof(unsigned char *) * byte);
					if (read_track_bytes(&track_dpos, m_track, variable_bytes, byte) < byte) {
						current_state = DONE;
						continue;
					}
					printf("Read [LYRICS]: %s\n", variable_bytes);
					break;
				case MARKER:
					// Length = variable
					if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
						current_state = DONE;
						continue;
					}

					// Marker data (byte length)
					variable_bytes = (unsigned char *) malloc(
							sizeof(unsigned char *) * byte);
					if (read_track_bytes(&track_dpos, m_track, variable_bytes, byte) < byte) {
						current_state = DONE;
						continue;
					}
					printf("Read [MARKER]: %s\n", variable_bytes);
					break;
				case TIME_SIGNATURE:
					// Length = 4 (always)
					if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
						current_state = DONE;
						continue;
					}

					// Time signature data (enforce length = 4)
					if (read_track_bytes(&track_dpos, m_track, data, byte) < 4) {
						current_state = DONE;
						continue;
					}
					time_numerator = data[0];
					m_file2->time_signature.numer = time_numerator;

					time_denominator = 2;
					for (i = 0; i < data[1]; ++i)
						time_denominator *= data[1];
					m_file2->time_signature.denom = time_denominator;

					metronome_ticks = data[2];
					m_file2->time_signature.metro = metronome_ticks;
					notes_pqn = data[3];
					m_file2->time_signature.notes_pqn = notes_pqn;

					printf("Read [TIME SIGNATURE]: %02x %02x %02x %02x\n",
							data[0], data[1], data[2], data[3]);
					break;
				case SMPTE_OFFSET:
					// Length = 5 (always)
					if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
						current_state = DONE;
						continue;
					}

					// SMPTE offset data (enforce length = 5)
					if (read_track_bytes(&track_dpos, m_track, data2, byte) < 5) {
						current_state = DONE;
						continue;
					}

					printf(
							"Read [SMPTE OFFSET]: Time: %d:%d:%d, Fr: %d, SubFr: %d\n",
							data2[0] & 0x2F, data2[1], data2[2], data2[3],
							data2[4]);
					break;
				case KEY_SIGNATURE:
					// Length = 2 (always)
					if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
						current_state = DONE;
						continue;
					}

					// Key signature data (enforce length = 2)
					if (read_track_bytes(&track_dpos, m_track, data, byte) < 2) {
						current_state = DONE;
						continue;
					}
					midi_key_signature = data[0];
					midi_scale = data[1];

					printf("Read [KEY SIGNATURE]: %02x %02x\n", data[0],
							data[1]);
					break;
				case END_OF_TRACK:
					// Length = 0 (always)
					printf("Read [END OF TRACK]: %02x %2x\n", code, byte);
					current_state = DONE;
					break;
				default:
					printf("Read [META_SYSTEM] code not recognized: %02x %02x\n", code, byte);

					// Length = variable
					if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
						current_state = DONE;
						continue;
					}

					// Unknown data (byte length)
					variable_bytes = (unsigned char *) malloc(sizeof(unsigned char) * byte);
					if (read_track_bytes(&track_dpos, m_track, variable_bytes, byte) < byte) {
						current_state = DONE;
						continue;
					}
					printf("Read [META_SYSTEM_UNKNOWN]: %s\n", variable_bytes);

					break;
				}
				break;
			case CONTROLLER:
				// Read data
				if (read_track_bytes(&track_dpos, m_track, bytes, 2) < 2) {
					current_state = DONE;
					continue;
				}

				data2[0] = 0x0B;
				data2[1] = 0xB0;
				data2[2] = bytes[0];
				data2[3] = bytes[1];

				snprintf(data, sizeof(data3), "%02x %02x %02x %02x\r\n",
						 data[0],data[1],data[2],data[3]);
				write(m_file2->connfd, data, strlen(data));
				printf("Wrote [Controller]: %02x %02x %02x %02x\n", data2[0],
						data2[1], data2[2], data2[3]);
				break;
			case POLY_AFTERTOUCH:
				// Read data
				if (read_track_bytes(&track_dpos, m_track, bytes, 2) < 2) {
					current_state = DONE;
					continue;
				}

				data[0] = 0x0A;
				data[1] = 0xA0;
				data[2] = bytes[0]; // Note number
				data[3] = bytes[1]; // Pressure

				snprintf(data, sizeof(data3), "%02x %02x %02x %02x\r\n",
						 data[0],data[1],data[2],data[3]);
				write(m_file2->connfd, data, strlen(data));
				printf("Wrote [Poly Aftertouch]: %02x %02x %02x %02x\n",
						data[0], data[1], data[2], data[3]);
				break;
			case PROGRAM_CHANGE:
				// Read data
				if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
					current_state = DONE;
					continue;
				}

				data[0] = 0x0C;
				data[1] = 0xC0;
				data[2] = byte; // Program number
				data[3] = 0;

				snprintf(data, sizeof(data3), "%02x %02x %02x %02x\r\n",
						 data[0],data[1],data[2],data[3]);
				write(m_file2->connfd, data, strlen(data));
				printf("Wrote [Program Change]: %02x %02x %02x\n", data[0],
						data[1], data[2]);

				break;
			default:
				printf("Read [CODE] code not recognized: %02x\n", code);

				// Length = variable
				if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
					current_state = DONE;
					continue;
				}

				// unknown data (byte length)
				variable_bytes = (unsigned char *) malloc(sizeof(unsigned char) * byte);
				if (read_track_bytes(&track_dpos, m_track, variable_bytes, byte) < byte) {
					current_state = DONE;
					continue;
				}
				printf("Read [UNKNOWN]: %s\n", variable_bytes);

				break;
			}
			if (delta_time > 0) {
				//printf("delaying usleep: %d ms bpm=%02x tdiv=%02x npn = %02x\n", delta_time*m_file2->bpm/m_file2->time_division, m_file2->bpm, m_file2->time_division,m_file2->time_signature.notes_pqn);
				/*delay(delta_time * m_file2->bpm
						/ m_file2->time_division);*/
				usleep(delta_time*m_file2->bpm*1000/m_file2->time_division);
			}
		}

		printf("Found [%d] tempo commands\n", i);
	} while (read_track_byte(&track_dpos, m_track, &byte) > 0 && current_track_num < num_tracks);
}

void *play_midi_track_udp(void *track_data)
{
     midi_track *m_track;
     m_track = (midi_track *)track_data;
     printf("Playing track #%d | size = %02x\n", m_track->track_num, m_track->size);

     printf("TIME SIGNATURE NPQ = %02x DIV = %02x BPM = %02x\n",
    		 m_file2->time_signature.notes_pqn, m_file2->time_division, m_file2->bpm);


     /* Start track playback */
     state current_state = START;

	unsigned char code, bytes[3], byte = 0, running_status;
	unsigned char *variable_bytes;
	unsigned int riff_len, i = 0, track_dpos = 0;
	unsigned int bpm, delta_time = 0;
	unsigned char data[100], data2[100], data3[100];
	unsigned short format, num_tracks, time_division;
	unsigned char time_numerator, time_denominator, metronome_ticks, notes_pqn;
	char midi_key_signature, midi_scale;
	int current_track_num = 0;
	unsigned char *variable_data;
	int skip_delta_time = 0;

	/*data3[0] = 0x0B;
	data3[1] = 0xB0;
	data3[2] = 0;
	data3[3] = 0;
	data3[4] = 0xB0;
	data3[5] = 1;
	data3[6] = 1;

	fwrite(data3, 7, 1, drv_ptr);
	fflush(drv_ptr);*/

	do {
		current_state = START;

		printf("delta time = %02x\n", m_track->delta_time);
		if (m_track->delta_time > 0) {
			//printf("delaying: %d ms\n",m_track->delta_time*m_file2->bpm/m_file2->time_division);
			usleep(m_track->delta_time*m_file2->bpm*1000/m_file2->time_division);
		}

		running_status = 0;
		while (current_state != DONE) {
			if(!skip_delta_time) {
				delta_time = 0;
				if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
					current_state = DONE;
					break;
				}
				delta_time = delta_time | byte;

				while ((byte & 0x80) == 0x80) {
					if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
						current_state = DONE;
						break;
					}
					delta_time = ((delta_time & 0x7F) << 7) | (byte & 0x7F);
				}

				printf("delta_time = %02x\n",delta_time);


				if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
					current_state = DONE;
					break;
				}
				printf("read code = %02x\n",byte);

				if ((byte & 0x80) == 0x80)
					running_status = 0;
				else
					running_status = 1;

				unread_track_byte(&track_dpos, m_track);
			} else {
				delta_time = m_track->delta_time;
				skip_delta_time = 0;
			}

			if (!running_status)
				if (read_track_byte(&track_dpos, m_track, &code) < 1) {
					current_state = DONE;
					break;
				}


			switch (code & 0xF0) {
			case NOTE_ON:
				if (read_track_bytes(&track_dpos, m_track, bytes, 2) < 2) {
					current_state = DONE;
					continue;
				}

				if (bytes[1] == 0) {
					//code = 0x80;
					data[0] = 0x08;
					data[1] = 0x80;
				} else {
					data[0] = 0x09;
					data[1] = 0x90;
				}

				data[2] = bytes[0];
				data[3] = bytes[1];


				printf("Wrote [Note On]: %02x %02x %02x %02x\n", data[0],
										data[1], data[2], data[3]);

				snprintf(data2, sizeof(data2), "%02x %02x %02x %02x\r\n",
						 data[0],data[1],data[2],data[3]);
				sendto(m_file2->s, data2, strlen(data2), 0, &(m_file2->si_other), m_file2->slen);

				break;
			case NOTE_OFF:
				if (read_track_bytes(&track_dpos, m_track, bytes, 2) < 2) {
					current_state = DONE;
					continue;
				}

				//code = 0x80;
				data[0] = 0x08;
				data[1] = 0x80;

				data[2] = bytes[0];
				data[3] = bytes[1];

				printf("Wrote [Note Off]: %02x %02x %02x %02x\n", data[0],
										data[1], data[2], data[3]);

				snprintf(data2, sizeof(data2), "%02x %02x %02x %02x\r\n",
						 data[0],data[1],data[2],data[3]);
				sendto(m_file2->s, data2, strlen(data2), 0, &(m_file2->si_other), m_file2->slen);
				break;
			case META_SYSTEM:
				// Type
				if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
					current_state = DONE;
					continue;
				}

				switch (byte) {
				case SET_TEMPO:
					// Length = 3 (always)
					if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
						printf("ending1\n");
						current_state = DONE;
						continue;
					}

					// Tempo data (enforce length = 3)
					if (read_track_bytes(&track_dpos, m_track, bytes, byte) < 3) {
						printf("ending2\n");
						current_state = DONE;
						continue;
					}
					bpm = ((((0 | bytes[0]) << 8) | bytes[1]) << 8) | bytes[2];
					bpm = bpm / 1000;
					m_file2->bpm = bpm;
					++i;
					printf("Detected [TEMPO CHANGE][%d]...\n", i);
					break;
				case TEXT:
					// Length = variable
					if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
						current_state = DONE;
						continue;
					}

					// Text data (byte length)
					variable_bytes = (unsigned char *) malloc(
							sizeof(unsigned char *) * byte);
					if (read_track_bytes(&track_dpos, m_track, variable_bytes, byte) < byte) {
						current_state = DONE;
						continue;
					}
					printf("Read [TEXT]: %s\n", variable_bytes);
					break;
				case COPYRIGHT:
					// Length = variable
					if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
						current_state = DONE;
						continue;
					}

					// Text data (byte length)
					variable_bytes = (unsigned char *) malloc(
							sizeof(unsigned char *) * byte);
					if (read_track_bytes(&track_dpos, m_track, variable_bytes, byte) < byte) {
						current_state = DONE;
						continue;
					}
					printf("Read [COPYRIGHT]: %s\n", variable_bytes);
					break;
				case TRACK_NAME:
					// Length = variable
					if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
						current_state = DONE;
						continue;
					}

					// Text data (byte length)
					variable_bytes = (unsigned char *) malloc(
							sizeof(unsigned char *) * byte);
					if (read_track_bytes(&track_dpos, m_track, variable_bytes, byte) < byte) {
						current_state = DONE;
						continue;
					}
					printf("Read [TRACK NAME]: %s\n", variable_bytes);
					break;
				case LYRICS:
					// Length = variable
					if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
						current_state = DONE;
						continue;
					}

					// Lyrics data (byte length)
					variable_bytes = (unsigned char *) malloc(
							sizeof(unsigned char *) * byte);
					if (read_track_bytes(&track_dpos, m_track, variable_bytes, byte) < byte) {
						current_state = DONE;
						continue;
					}
					printf("Read [LYRICS]: %s\n", variable_bytes);
					break;
				case MARKER:
					// Length = variable
					if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
						current_state = DONE;
						continue;
					}

					// Marker data (byte length)
					variable_bytes = (unsigned char *) malloc(
							sizeof(unsigned char *) * byte);
					if (read_track_bytes(&track_dpos, m_track, variable_bytes, byte) < byte) {
						current_state = DONE;
						continue;
					}
					printf("Read [MARKER]: %s\n", variable_bytes);
					break;
				case TIME_SIGNATURE:
					// Length = 4 (always)
					if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
						current_state = DONE;
						continue;
					}

					// Time signature data (enforce length = 4)
					if (read_track_bytes(&track_dpos, m_track, data, byte) < 4) {
						current_state = DONE;
						continue;
					}
					time_numerator = data[0];
					m_file2->time_signature.numer = time_numerator;

					time_denominator = 2;
					for (i = 0; i < data[1]; ++i)
						time_denominator *= data[1];
					m_file2->time_signature.denom = time_denominator;

					metronome_ticks = data[2];
					m_file2->time_signature.metro = metronome_ticks;
					notes_pqn = data[3];
					m_file2->time_signature.notes_pqn = notes_pqn;

					printf("Read [TIME SIGNATURE]: %02x %02x %02x %02x\n",
							data[0], data[1], data[2], data[3]);
					break;
				case SMPTE_OFFSET:
					// Length = 5 (always)
					if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
						current_state = DONE;
						continue;
					}

					// SMPTE offset data (enforce length = 5)
					if (read_track_bytes(&track_dpos, m_track, data2, byte) < 5) {
						current_state = DONE;
						continue;
					}

					printf(
							"Read [SMPTE OFFSET]: Time: %d:%d:%d, Fr: %d, SubFr: %d\n",
							data2[0] & 0x2F, data2[1], data2[2], data2[3],
							data2[4]);
					break;
				case KEY_SIGNATURE:
					// Length = 2 (always)
					if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
						current_state = DONE;
						continue;
					}

					// Key signature data (enforce length = 2)
					if (read_track_bytes(&track_dpos, m_track, data, byte) < 2) {
						current_state = DONE;
						continue;
					}
					midi_key_signature = data[0];
					midi_scale = data[1];

					printf("Read [KEY SIGNATURE]: %02x %02x\n", data[0],
							data[1]);
					break;
				case END_OF_TRACK:
					// Length = 0 (always)
					printf("Read [END OF TRACK]: %02x %2x\n", code, byte);
					current_state = DONE;
					break;
				default:
					printf("Read [META_SYSTEM] code not recognized: %02x %02x\n", code, byte);

					// Length = variable
					if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
						current_state = DONE;
						continue;
					}

					// Unknown data (byte length)
					variable_bytes = (unsigned char *) malloc(sizeof(unsigned char) * byte);
					if (read_track_bytes(&track_dpos, m_track, variable_bytes, byte) < byte) {
						current_state = DONE;
						continue;
					}
					printf("Read [META_SYSTEM_UNKNOWN]: %s\n", variable_bytes);

					break;
				}
				break;
			case CONTROLLER:
				// Read data
				if (read_track_bytes(&track_dpos, m_track, bytes, 2) < 2) {
					current_state = DONE;
					continue;
				}

				data[0] = 0x0B;
				data[1] = 0xB0;
				data[2] = bytes[0];
				data[3] = bytes[1];

				snprintf(data2, sizeof(data2), "%02x %02x %02x %02x\r\n",
						data[0],data[1],data[2],data[3]);
				sendto(m_file2->s, data2, strlen(data2), 0, &(m_file2->si_other), m_file2->slen);
				printf("Wrote [Controller]: %02x %02x %02x %02x\n", data[0],
						data[1], data[2], data[3]);
				break;
			case POLY_AFTERTOUCH:
				// Read data
				if (read_track_bytes(&track_dpos, m_track, bytes, 2) < 2) {
					current_state = DONE;
					continue;
				}

				data[0] = 0x0A;
				data[1] = 0xA0;
				data[2] = bytes[0]; // Note number
				data[3] = bytes[1]; // Pressure

				snprintf(data2, sizeof(data2), "%02x %02x %02x %02x\r\n",
						 data[0],data[1],data2[2],data2[3]);
				sendto(m_file2->s, data2, strlen(data2), 0, &(m_file2->si_other), m_file2->slen);
				printf("Wrote [Poly Aftertouch]: %02x %02x %02x %02x\n",
						data[0], data[1], data[2], data[3]);
				break;
			case PROGRAM_CHANGE:
				// Read data
				if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
					current_state = DONE;
					continue;
				}

				data[0] = 0x0C;
				data[1] = 0xC0;
				data[2] = byte; // Program number
				data[3] = 0;

				printf("Wrote [Program Change]: %02x %02x %02x %02x\n", data[0],
						data[1], data[2], data[3]);
				snprintf(data2, sizeof(data2), "%02x %02x %02x %02x\r\n",
						data[0],data[1],data[2],data[3]);
				sendto(m_file2->s, data2, strlen(data2), 0, &(m_file2->si_other), m_file2->slen);
				//return -1;

				break;
			default:
				printf("Read [CODE] code not recognized: %02x\n", code);

				// Length = variable
				if (read_track_byte(&track_dpos, m_track, &byte) < 1) {
					current_state = DONE;
					continue;
				}

				// unknown data (byte length)
				variable_bytes = (unsigned char *) malloc(sizeof(unsigned char) * byte);
				if (read_track_bytes(&track_dpos, m_track, variable_bytes, byte) < byte) {
					current_state = DONE;
					continue;
				}
				printf("Read [UNKNOWN]: %s\n", variable_bytes);

				break;
			}
			if (delta_time > 0) {
				//printf("delaying usleep: %d ms bpm=%02x tdiv=%02x npn = %02x\n", delta_time*m_file2->bpm/m_file2->time_division, m_file2->bpm, m_file2->time_division,m_file2->time_signature.notes_pqn);
				/*delay(delta_time * m_file2->bpm
						/ m_file2->time_division);*/
				usleep(delta_time*m_file2->bpm*1000/m_file2->time_division);
			}
		}

		printf("Found [%d] tempo commands\n", i);
	} while (read_track_byte(&track_dpos, m_track, &byte) > 0 && current_track_num < num_tracks);
}

int parseFile(char *filename, midi_file **m_file) {
	unsigned int riff_len, delta_time;
	unsigned short current_track_num;
	unsigned char byte, data[4];
	FILE *midi_fp;
	midi_track *m_tracks;

	midi_fp = fopen(filename, "rb");
	if (midi_fp == NULL ) {
		printf("Could not open file: %s\n", filename);
		return -1;
	}

	*m_file = (midi_file *)malloc(sizeof(midi_file));

	// Parse header
	// RIFF Chunk MThd
	if (fread(data, 1, 4, midi_fp) < 4) {
		printf("Not a midi file\n");
		return -1;
	}
	if (memcmp(data, "MThd", 4) != 0) {
		printf("Not a midi file\n");
		return -1;
	}
	printf("got: %02x %02x %02x %02x\n",data[0],data[1],data[2],data[3]);

	// RIFF Chunk length
	if (fread(data, 1, 4, midi_fp) < 4) {
		printf("Corrupt midi file 1\n");
		return -1;
	}
	printf("got: %02x %02x %02x %02x\n",data[0],data[1],data[2],data[3]);
	riff_len = ((((((0 | data[0]) << 8) | data[1]) << 8) | data[2]) << 8)
			| data[3];
	if (riff_len != 6) {
		printf("Corrupt midi file. Expect length of 6 but got %d\n", riff_len);
		return -1;
	}

	// Midi format
	if (fread(data, 1, 2, midi_fp) < 2) {
		printf("Corrupt midi file 3\n");
		return -1;
	}
	printf("got: %02x %02x\n",data[0],data[1]);

	(*m_file)->format = ((0 | data[0]) << 8) | data[1];

	// Midi number of tracks
	if (fread(data, 1, 2, midi_fp) < 2) {
		printf("Corrupt midi file 4\n");
		return -1;
	}
	(*m_file)->num_tracks = ((0 | data[0]) << 8) | data[1];
	printf("got: %02x %02x\n",data[0],data[1]);

	// Midi time division
	if (fread(data, 1, 2, midi_fp) < 2) {
		printf("Corrupt midi file 5\n");
		return -1;
	}
	(*m_file)->time_division = ((0 | data[0]) << 8) | data[1];
	printf("got: %02x %02x\n",data[0],data[1]);

	// Allocate tracks
	m_tracks = (midi_track *) malloc(sizeof(midi_track) * (*m_file)->num_tracks);

	byte = 0;
	current_track_num = 0;
	do {
		// Parse track delta time
		delta_time = 0;
		delta_time = delta_time | byte;

		while ((byte & 0x80) == 0x80) {
			if (fread(&byte, 1, 1, midi_fp) < 1)
				break;

			delta_time = ((delta_time & 0x7F) << 7) | (byte & 0x7F);
		}


		m_tracks[current_track_num].delta_time = delta_time;
		printf("Parsing Track #%d\n", current_track_num + 1);

		// RIFF Chunk MTrk
		if (fread(data, 1, 4, midi_fp) < 4) {
			printf("Corrupt midi file 6\n");
			return -1;
		}
		printf("MTrk: %02x %02x %02x %02x\n", data[0], data[1], data[2],
				data[3]);
		if (memcmp(data, "MTrk", 4) != 0) {
			printf("No tracks found\n");
			return -1;
		}

		// RIFF Chunk length
		if (fread(data, 1, 4, midi_fp) < 4) {
			printf("Corrupt midi file 7\n");
			return -1;
		}
		printf("Reading next: %02x %02x %02x %02x bytes for data\n",
				data[0],data[1],data[2],data[3]);
		riff_len = (((((((0 | data[0]) << 8) | data[1]) << 8) | data[2]) << 8)
				| data[3]) - 1;

		m_tracks[current_track_num].size = riff_len;

		m_tracks[current_track_num].track_num = current_track_num + 1;

		m_tracks[current_track_num].data = (unsigned char *) malloc(sizeof(unsigned char)*riff_len);
		if (fread(m_tracks[current_track_num].data, 1, riff_len, midi_fp) < riff_len) {
			printf("Error track terminated before %d bytes.\n", riff_len);
			return -1;
		}
		printf("READ: %02x %02x %02x %02x bytes for data\n",
						data[0],data[1],data[2],data[3]);
		printf("m_tracks[%d].data (size=%02x) == data!\n",current_track_num,riff_len);
	} while (fread(&byte, 1, 1, midi_fp) == 1 && ++current_track_num < (*m_file)->num_tracks);

	(*m_file)->tracks = m_tracks;

	fclose(midi_fp);
	printf("done\n");

	return 1;
}

void playMidi()
{
	if(m_file2 == NULL)
		return;

	unsigned char data3[4];

	// Reset system
	data3[0] = 0x0F;
	data3[1] = 0xFF;
	data3[2] = 0x00;
	data3[3] = 0x00;

	fwrite(data3, 4, 1, m_file2->drv_ptr);
	fflush(m_file2->drv_ptr);

	usleep(1000*5000);

	// Turn on pedals
	data3[0] = 0x0B;
	data3[1] = 0xB0;
	data3[2] = 0x40;
	data3[3] = 0x7F;

	fwrite(data3, 4, 1, m_file2->drv_ptr);
	fflush(m_file2->drv_ptr);

	printf("Playing Midi...\n");

	pthread_t *threads;
	int *t_rets, i;

	threads = (pthread_t *)malloc(sizeof(pthread_t)*m_file2->num_tracks);
	t_rets = (int *)malloc(sizeof(int)*m_file2->num_tracks);

	m_file2->bpm = 1200;

	// Create independent threads each of which will execute function
	for(i=0; i<m_file2->num_tracks; ++i)
		t_rets[i] = pthread_create( &threads[i], NULL, play_midi_track, &(m_file2->tracks[i]));

	// Create independent threads each of which will execute function
	for(i=0; i<m_file2->num_tracks; ++i) {
		// Wait till threads are complete before main continues.
		pthread_join( threads[i], NULL);
		printf("Waiting for track #%d.\n", i+1);
	}

	// Turn off pedals
	data3[0] = 0x0B;
	data3[1] = 0xB0;
	data3[2] = 0x40;
	data3[3] = 0;

	fwrite(data3, 4, 1, m_file2->drv_ptr);
	fflush(m_file2->drv_ptr);

	printf("Midi playback complete.\n");
}

void playMidi_net()
{
	if(m_file2->connfd == -1)
		return;

	unsigned char data3[100];

	// Reset system
	data3[0] = 0x0F;
	data3[1] = 0xFF;
	data3[2] = 0x00;
	data3[3] = 0x00;

	snprintf(data3, sizeof(data3), "%02x %02x %02x %02x\r\n",
			 data3[0],data3[1],data3[2],data3[3]);

	write(m_file2->connfd, data3, strlen(data3));

	usleep(1000*5000);

	// Turn on pedals
	data3[0] = 0x0B;
	data3[1] = 0xB0;
	data3[2] = 0x40;
	data3[3] = 0x7F;

	snprintf(data3, sizeof(data3), "%02x %02x %02x %02x\r\n",
			 data3[0],data3[1],data3[2],data3[3]);
	write(m_file2->connfd, data3, strlen(data3));

	usleep(1000*5000);

	printf("Playing Midi...\n");

	pthread_t *threads;
	int *t_rets, i;

	threads = (pthread_t *)malloc(sizeof(pthread_t)*m_file2->num_tracks);
	t_rets = (int *)malloc(sizeof(int)*m_file2->num_tracks);

	m_file2->bpm = 1200;

	// Create independent threads each of which will execute function
	for(i=0; i<m_file2->num_tracks; ++i)
		t_rets[i] = pthread_create( &threads[i], NULL, play_midi_track_net, &(m_file2->tracks[i]));

	// Create independent threads each of which will execute function
	for(i=0; i<m_file2->num_tracks; ++i) {
		// Wait till threads are complete before main continues.
		pthread_join( threads[i], NULL);
		printf("Waiting for track #%d.\n", i+1);
	}

	usleep(1000*5000);
	// Turn off pedals
	data3[0] = 0x0B;
	data3[1] = 0xB0;
	data3[2] = 0x40;
	data3[3] = 0;

	snprintf(data3, sizeof(data3), "%02x %02x %02x %02x\r\n",
			 data3[0],data3[1],data3[2],data3[3]);
	write(m_file2->connfd, data3, strlen(data3));

	printf("Midi playback complete.\n");
}

void playMidi_udp()
{
	if(m_file2->s == -1)
		return;

	unsigned char data3[100];
	unsigned char data2[100];

	usleep(1000*2000);

	// Turn on pedals
	data2[0] = 0x0B;
	data2[1] = 0xB0;
	data2[2] = 0x40;
	data2[3] = 0x7F;

	snprintf(data3, sizeof(data3), "%02x %02x %02x %02x\r\n",
			 data2[0],data2[1],data2[2],data2[3]);

	usleep(1000*2000);

	printf("Playing Midi...\n");

	pthread_t *threads;
	int *t_rets, i;

	threads = (pthread_t *)malloc(sizeof(pthread_t)*m_file2->num_tracks);
	t_rets = (int *)malloc(sizeof(int)*m_file2->num_tracks);

	m_file2->bpm = 1200;

	// Create independent threads each of which will execute function
	for(i=0; i<m_file2->num_tracks; ++i)
		t_rets[i] = pthread_create( &threads[i], NULL, play_midi_track_udp, &(m_file2->tracks[i]));

	// Create independent threads each of which will execute function
	for(i=0; i<m_file2->num_tracks; ++i) {
		// Wait till threads are complete before main continues.
		pthread_join( threads[i], NULL);
		printf("Waiting for track #%d.\n", i+1);
	}

	usleep(1000*5000);
	// Turn off pedals
	data2[0] = 0x0B;
	data2[1] = 0xB0;
	data2[2] = 0x40;
	data2[3] = 0;

	snprintf(data3, sizeof(data3), "%02x %02x %02x %02x\r\n",
			 data2[0],data2[1],data2[2],data2[3]);
	sendto(m_file2->s, data3, strlen(data3), 0, &(m_file2->si_other), m_file2->slen);

	printf("Midi playback complete.\n");
}
int main(int argc, char *argv[]) {
	FILE *fp = NULL;

	fp = fopen(argv[1], "wb");
	if (fp == NULL ) {
		puts("Could not open device\n");
		return EXIT_FAILURE;
	}

	//if (initialize(argv[3]) == -1) {
		//printf("Could not open midi file!\n");
		//return EXIT_FAILURE;
	//}
	printf("The device have been opened!\n");

	clock_t start_t;
	double elapsedTime = 0;

	start_t = clock();

	parseFile(argv[3], &m_file2);
	m_file2->drv_ptr = fp;
	playMidi(fp, (int) strtol(argv[2], NULL, 0));
	//playMidi();
	//playMidi2(fp, (int) strtol(argv[2], NULL, 0));

	elapsedTime = (double) (clock() - start_t) / (CLOCKS_PER_SEC);
	printf("The song went on for: %f seconds = %f minutes\n", elapsedTime,
			elapsedTime / 60);



	//fclose(fp);

	//deinitialize();

	return EXIT_SUCCESS;
}

// Using the custom driver, this
// main will send the signals
// over to a TCP server.
/*int main(int argc, char *argv[])
{
    int listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr;

    char sendBuff[1025];
    time_t ticks;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));
    memset(sendBuff, '0', sizeof(sendBuff));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(4444);

    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    listen(listenfd, 10);


    unsigned char data3[100];

    while(1)
    {
        connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);

        printf("New client connected!\n");

		clock_t start_t;
		double elapsedTime = 0;

		start_t = clock();

		//playMidi(fp, (int) strtol(argv[2], NULL, 0));
		parseFile("/home/haleeq/Desktop/dev/drivers/SmartMusic/music/Narutotheme.mid", &m_file2);
		m_file2->connfd = connfd;
		playMidi_net();

		elapsedTime = (double) (clock() - start_t) / (CLOCKS_PER_SEC);
		printf("The song went on for: %f seconds = %f minutes\n", elapsedTime,
            			elapsedTime / 60);

        ticks = time(NULL);
        //snprintf(sendBuff, sizeof(sendBuff), "%s\r\n", "09 90 1c 60");
        //write(connfd, sendBuff, strlen(sendBuff));

        close(connfd);
        sleep(1);
     }
}*/

// Using the custom usb driver,
// This main will send the signals
// over to a UDP server.
/*
#define BUFLEN 512
#define NPACK 10
#define PORT 4444

void diep(char *s)
{
	perror(s);
	exit(1);
}

int main(void)
{
	struct sockaddr_in si_me, si_other;
	int s, i, slen=sizeof(si_other);
	char buf[BUFLEN];

	struct pollfd pfds[1];
	int fd, avail_data;
	unsigned char buf2[1024], data3[100];

	if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
		diep("socket");
	else
		printf("Successfully created socket.\n");

	memset((char *) &si_me, 0, sizeof(si_me));
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(PORT);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(s, &si_me, sizeof(si_me))==-1)
		diep("bind");
	else
		printf("Successfully binded to port.\n");

	while(1) {

		if(recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *)&si_other, &slen)==-1)
			diep("recvfrom()");
		printf("Received packet from %s:%d\nData: %s\n\n",
				inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port), buf);

		if(strcmp(buf,"START") != 0)
			continue;

		clock_t start_t;
		double elapsedTime = 0;

		start_t = clock();

		parseFile("/home/haleeq/Desktop/dev/drivers/SmartMusic/music/ZeldaLostWoodsPianoDuet.mid", &m_file2);
		m_file2->s = s;
		m_file2->si_other = si_other;
		m_file2->slen = slen;
		playMidi_udp();

		fd = open("/dev/skel0", O_RDONLY);
		if (fd == -1 ) {
			printf("Could not open device\n");
			return EXIT_FAILURE;
		}
		printf("Opened deviced!\n");

		// Turn on pedals
		data3[0] = 0x0B;
		data3[1] = 0xB0;
		data3[2] = 0x40;
		data3[3] = 0x7F;

		sendto(s, data3, 4, 0, &si_other, slen);

		while(1) {
			pfds[0].fd = fd;
			pfds[0].events = POLLIN;

			poll(pfds, 1, -1);

			if(pfds[0].revents & POLLIN) {
				avail_data = read(fd, buf2, 1024);
				if(!avail_data) {
					printf("device closed!\n");
					break;
				}
				if(avail_data > 0) {
					sendto(s, buf2, 4, 0, &si_other, slen);
					printf("[SENT DEVICE DATA]: ");
					//for(i=0; i<avail_data; ++i)
						//printf(" %02x", buf2[i]);
					//printf("\n");
				}
			}
		}

		close(fd);

		elapsedTime = (double) (clock() - start_t) / (CLOCKS_PER_SEC);
		printf("The song went on for: %f seconds = %f minutes\n", elapsedTime,
	            				elapsedTime / 60);
	}

	close(s);

	return 0;
}*/


// Alternative Main using alsa
// Signals from the Midi device are sent
// to a UDP server.
/*
static void usage(void)
{
	fprintf(stderr, "usage: rawmidi [options]\n");
	fprintf(stderr, " options:\n");
	fprintf(stderr, " -v: verbose mode\n");
	fprintf(stderr, " -i device-id : test ALSA input device\n");
	fprintf(stderr, " -o device-id : test ALSA output device\n");
	fprintf(stderr, " -I node : test input node\n");
	fprintf(stderr, " -O node : test output node\n");
	fprintf(stderr, " -t: test midi thru\n");
	fprintf(stderr, " example:\n");
	fprintf(stderr, " rawmidi -i hw:0,0 -O /dev/midi1\n");
	fprintf(stderr, " tests input for card 0, device 0, using snd_rawmidi API\n");
	fprintf(stderr, " and /dev/midi1 using file descriptors\n");
}

int stop=0;
void sighandler(int dum)
{
	stop=1;
}

int main(int argc,char** argv)
{
	int i;
	int err;
	int verbose = 1;
	char *device_in = NULL;
	char *device_out = NULL;
	char *node_in = NULL;
	char *node_out = NULL;
	int fd_in = -1,fd_out = -1;
	snd_rawmidi_t *handle_in = 0,*handle_out = 0;
	if (argc==1) {
		usage();
		exit(0);
	}
	for (i = 1 ; i<argc ; i++) {
		if (argv[i][0]=='-') {
			switch (argv[i][1]) {
			case 'h':
				usage();
				break;
			case 'v':
				verbose = 1;
				break;
			case 'i':
				if (i + 1 < argc)
					device_in = argv[++i];
				break;
			case 'I':
				if (i + 1 < argc)
					node_in = argv[++i];
				break;
			case 'o':
				if (i + 1 < argc)
					device_out = argv[++i];
				break;
			case 'O':
				if (i + 1 < argc)
					node_out = argv[++i];
				break;
			}
		}
	}
	if (verbose) {
		fprintf(stderr,"Using: \n");
		fprintf(stderr,"Input: ");
		if (device_in) {
			fprintf(stderr,"device %s\n",device_in);
		}else if (node_in){
			fprintf(stderr,"%s\n",node_in);
		}else{
			fprintf(stderr,"NONE\n");
		}
		fprintf(stderr,"Output: ");
		if (device_out) {
			fprintf(stderr,"device %s\n",device_out);
		}else if (node_out){
			fprintf(stderr,"%s\n",node_out);
		}else{
			fprintf(stderr,"NONE\n");
		}
	}
	if (device_in) {
		err = snd_rawmidi_open(&handle_in,NULL,device_in,0);
		if (err) {
			fprintf(stderr,"snd_rawmidi_open %s failed: %d\n",device_in,err);
		}
	}
	if (node_in && (!node_out || strcmp(node_out,node_in))) {
		fd_in = open(node_in,O_RDONLY);
		if (fd_in<0) {
			fprintf(stderr,"open %s for input failed\n",node_in);
		}
	}
	//if (device_out) {
		//err = snd_rawmidi_open(NULL,&handle_out,device_out,0);
		//if (err) {
			//fprintf(stderr,"snd_rawmidi_open %s failed: %d\n",device_out,err);
		//}
	//}
	//if (node_out && (!node_in || strcmp(node_out,node_in))) {
		//fd_out = open(node_out,O_WRONLY);
		//if (fd_out<0) {
			//fprintf(stderr,"open %s for output failed\n",node_out);
		//}
	//}
	//if (node_in && node_out && strcmp(node_out,node_in)==0) {
		//fd_in = fd_out = open(node_out,O_RDWR);
		//if (fd_out<0) {
			//fprintf(stderr,"open %s for input and output failed\n",node_out);
		//}
	//}
	if ((handle_in || fd_in!=-1)) {
		if (verbose) {
			fprintf(stderr,"Testing midi thru in\n");
		}



		struct sockaddr_in si_me, si_other;
		int s, i, slen=sizeof(si_other);
		char buf[BUFLEN];

		struct pollfd pfds[1];
		int fd, avail_data;
		unsigned char buf2[1024], data3[4];

		if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
			diep("socket");
		else
			printf("Successfully created socket.\n");

		memset((char *) &si_me, 0, sizeof(si_me));
		si_me.sin_family = AF_INET;
		si_me.sin_port = htons(PORT);
		si_me.sin_addr.s_addr = htonl(INADDR_ANY);
		if (bind(s, &si_me, sizeof(si_me))==-1)
			diep("bind");
		else
			printf("Successfully binded to port.\n");

		while(1) {

			if(recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *)&si_other, &slen)==-1)
				diep("recvfrom()");
			printf("Received packet from %s:%d\nData: %s\n\n",
					inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port), buf);

			//if(strcmp(buf,"START") != 0)
				//continue;

			data3[0] = 0x0B;
			data3[1] = 0xB0;
			data3[2] = 0x40;
			data3[3] = 0x7F;

			sendto(s, data3, 4, 0, &si_other, slen);

			clock_t start_t;
			double elapsedTime = 0;

			start_t = clock();
			i = 0;

			//parseFile("/home/haleeq/Desktop/dev/drivers/SmartMusic/music/CircleOfLife-6.mid", &m_file2);
			//m_file2->s = s;
			//m_file2->si_other = si_other;
			//m_file2->slen = slen;
			//playMidi_udp();

			unsigned char last_ch;
			while (1) {
				unsigned char ch;
				//int i33;
				if (handle_in) {
					//printf("[READ]: ");
					snd_rawmidi_read(handle_in,data3+1,3);
					//for(i33=1; i33<4; ++i33)
						//printf(" %02x", data3[i33]);
					//printf("\n");
					if(data3[1]==0x90) {
						if(data3[3]==0x00) {
							data3[0] = 0x08;
							data3[1] = 0x80;
						} else
							data3[0] = 0x09;
					} else if(data3[1]==0x80) {
						data3[0] = 0x08;
					} else if(data3[1]==0xB0) {
						data3[0] = 0x0B;
					} else if(data3[1]==0xC0) {
						data3[0] = 0x0C;
					}

					sendto(s, data3, 4, 0, &si_other, slen);
					printf("[SENT DEVICE DATA]: %02x %02x %02x %02x\n",
							data3[0],data3[1],data3[2],data3[3]);
				}
				if (fd_in!=-1) {
					read(fd_in,&ch,1);
				}
				//if (verbose) {
					//fprintf(stderr,"thru: %02x\n",ch);
				//}
				//if (handle_out) {
					//snd_rawmidi_write(handle_out,data3+1,3);
					//snd_rawmidi_drain(handle_out);
				//}
				//if (fd_out!=-1) {
					//write(fd_out,&ch,1);
				//}
			}

			elapsedTime = (double) (clock() - start_t) / (CLOCKS_PER_SEC);
			printf("The song went on for: %f seconds = %f minutes\n", elapsedTime,
									elapsedTime / 60);
		}

		close(s);

	}else{
		fprintf(stderr,"Testing midi thru needs both input and output\n");
		exit(-1);
	}

	if (verbose) {
		fprintf(stderr,"Closing\n");
	}
	if (handle_in) {
		snd_rawmidi_drain(handle_in);
		snd_rawmidi_close(handle_in);
	}
	//if (handle_out) {
		//snd_rawmidi_drain(handle_out);
		//snd_rawmidi_close(handle_out);
	//}a
	if (fd_in!=-1) {
		close(fd_in);
	}
	//if (fd_out!=-1) {
		//close(fd_out);
	//}
	return 0;
}*/
