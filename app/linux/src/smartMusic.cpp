/*
 ============================================================================
 Name        : smartMusic.c
 Author      : Haleeq Usman
 Version     : 0.0
 Description : Smart Music Application (dev)
 ============================================================================
 */
#include <time.h>
#include <stdio.h>
#include "include/ByteReader.h"

void delay(double ms) {
	clock_t start_t;

	start_t = clock();
	while ((double) (clock() - start_t) / (CLOCKS_PER_SEC / 1000) < ms);
}

midi_file *m_file;


void udp_stream(unsigned char *midi_ascii_data) {
	sendto(m_file->s, midi_ascii_data, strlen((char *)midi_ascii_data), 0, (sockaddr *)&(m_file->si_other), m_file->slen);
}

void midi_command_to_ascii(unsigned char *midi_data, unsigned char *midi_ascii_data) {
	char midi_command_format[] = "%02x %02x %02x %02x\r\n";
	snprintf((char *)midi_ascii_data, strlen(midi_command_format), midi_command_format,
			midi_data[0],
			midi_data[1],
			midi_data[2],
			midi_data[3]
	);
}

void decode_next_state(state *current_state, unsigned char code, ByteReader *reader) {
	unsigned char read_bytes[3], read_byte = 0, midi_data[4];
	unsigned char *text_data;
	unsigned int i = 0;
	unsigned char midi_ascii_data[100];
	unsigned char time_numerator, time_denominator, metronome_ticks, notes_pqn;
	unsigned int bpm = 0;

	switch (code & 0xF0) {
		case NOTE_ON:
			if(reader->read_track_bytes(read_bytes, 2) < 2) {
				*current_state = DONE;
				return;
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

			printf("Wrote [Note On]: %02x %02x %02x %02x\n", midi_data[0], midi_data[1], midi_data[2], midi_data[3]);

			break;
		case NOTE_OFF:
			if(reader->read_track_bytes(read_bytes, 2) < 2) {
				*current_state = DONE;
				return;
			}

			//code = 0x80;
			midi_data[0] = 0x08;
			midi_data[1] = 0x80;

			midi_data[2] = read_bytes[0];
			midi_data[3] = read_bytes[1];

			printf("Wrote [Note Off]: %02x %02x %02x %02x\n", midi_data[0], midi_data[1], midi_data[2], midi_data[3]);

		break;
		case META_SYSTEM:
			// Type
			if(reader->read_track_byte(&read_byte) < 1) {
				*current_state = DONE;
				return;
			}

			switch (read_byte) {
				case SET_TEMPO:
					// Length = 3 (always)
					if(reader->read_track_byte(&read_byte) < 1) {
						printf("ending1\n");
						*current_state = DONE;
						return;
					}

					// Tempo data (enforce length = 3)
					if(reader->read_track_bytes(read_bytes, read_byte) < 3) {
						printf("ending2\n");
						*current_state = DONE;
						return;
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
					*current_state = DONE;
					return;
				}

				// Text data (byte length)
				text_data = (unsigned char *) malloc(sizeof(unsigned char *) * read_byte);
				if(reader->read_track_bytes(text_data, read_byte) < read_byte) {
					*current_state = DONE;
					return;
				}
				printf("Read [TEXT]: %s\n", text_data);
			break;
			case COPYRIGHT:
				// Length = variable
				if(reader->read_track_byte(&read_byte) < 1) {
					*current_state = DONE;
					return;
				}

				// Text data (byte length)
				text_data = (unsigned char *) malloc(sizeof(unsigned char *) * read_byte);
					if(reader->read_track_bytes(text_data, read_byte) < read_byte) {
					*current_state = DONE;
					return;
				}
				printf("Read [COPYRIGHT]: %s\n", text_data);
			break;
			case TRACK_NAME:
				// Length = variable
				if(reader->read_track_byte(&read_byte) < 1) {
					*current_state = DONE;
					return;
				}

				// Text data (byte length)
				text_data = (unsigned char *) malloc(sizeof(unsigned char *) * read_byte);
					if(reader->read_track_bytes(text_data, read_byte) < read_byte) {
					*current_state = DONE;
					return;
				}
				printf("Read [TRACK NAME]: %s\n", text_data);
			break;
			case LYRICS:
				// Length = variable
				if(reader->read_track_byte(&read_byte) < 1) {
					*current_state = DONE;
					return;
				}

				// Lyrics data (byte length)
				text_data = (unsigned char *) malloc(sizeof(unsigned char *) * read_byte);
					if(reader->read_track_bytes(text_data, read_byte) < read_byte) {
					*current_state = DONE;
					return;
				}
				printf("Read [LYRICS]: %s\n", text_data);
			break;
			case MARKER:
				// Length = variable
				if(reader->read_track_byte(&read_byte) < 1) {
					*current_state = DONE;
					return;
				}

				// Marker data (byte length)
				text_data = (unsigned char *) malloc(sizeof(unsigned char *) * read_byte);
					if(reader->read_track_bytes(text_data, read_byte) < read_byte) {
					*current_state = DONE;
					return;
				}
				printf("Read [MARKER]: %s\n", text_data);
			break;
			case TIME_SIGNATURE:
				// Length = 4 (always)
				if(reader->read_track_byte(&read_byte) < 1) {
					*current_state = DONE;
					return;
				}

				// Time signature data (enforce length = 4)
				if(reader->read_track_bytes(midi_data, read_byte) < 4) {
					*current_state = DONE;
					return;
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

				printf("Read [TIME SIGNATURE]: %02x %02x %02x %02x\n", midi_data[0], midi_data[1], midi_data[2], midi_data[3]);
			break;
			case SMPTE_OFFSET:
				// Length = 5 (always)
				if(reader->read_track_byte(&read_byte) < 1) {
					*current_state = DONE;
					return;
				}

				// SMPTE offset data (enforce length = 5)
				if(reader->read_track_bytes(midi_ascii_data, read_byte) < 5) {
					*current_state = DONE;
					return;
				}

				printf(
				"Read [SMPTE OFFSET]: Time: %d:%d:%d, Fr: %d, SubFr: %d\n",
				midi_ascii_data[0] & 0x2F, midi_ascii_data[1], midi_ascii_data[2], midi_ascii_data[3], midi_ascii_data[4]);
			break;
			case KEY_SIGNATURE:
				// Length = 2 (always)
				if(reader->read_track_byte(&read_byte) < 1) {
					*current_state = DONE;
					return;
				}

				// Key signature data (enforce length = 2)
				if(reader->read_track_bytes(midi_data, read_byte) < 2) {
					*current_state = DONE;
					return;
				}

				printf("Read [KEY SIGNATURE]: %02x %02x\n", midi_data[0], midi_data[1]);
			break;
			case END_OF_TRACK:
				// Length = 0 (always)
				printf("Read [END OF TRACK]: %02x %2x\n", code, read_byte);
				*current_state = DONE;
			break;
			default:
				printf("Read [META_SYSTEM] code not recognized: %02x %02x\n", code, read_byte);

				// Length = variable
					if(reader->read_track_byte(&read_byte) < 1) {
					*current_state = DONE;
					return;
				}

				// Unknown data (byte length)
				text_data = (unsigned char *) malloc(sizeof(unsigned char) * read_byte);
				if(reader->read_track_bytes(text_data, read_byte) < read_byte) {
					*current_state = DONE;
					return;
				}
				printf("Read [META_SYSTEM_UNKNOWN]: %s\n", text_data);
			break;
		}
		break;
		case CONTROLLER:
			// Read data
			if(reader->read_track_bytes(read_bytes, 2) < 2) {
				*current_state = DONE;
				return;
			}

			midi_data[0] = 0x0B;
			midi_data[1] = 0xB0;
			midi_data[2] = read_bytes[0];
			midi_data[3] = read_bytes[1];

			printf("Wrote [Controller]: %02x %02x %02x %02x\n", midi_data[0], midi_data[1], midi_data[2], midi_data[3]);
		break;
		case POLY_AFTERTOUCH:
			// Read data
			if(reader->read_track_bytes(read_bytes, 2) < 2) {
				*current_state = DONE;
				return;
			}

			midi_data[0] = 0x0A;
			midi_data[1] = 0xA0;
			midi_data[2] = read_bytes[0]; // Note number
			midi_data[3] = read_bytes[1]; // Pressure

			printf("Wrote [Poly Aftertouch]: %02x %02x %02x %02x\n", midi_data[0], midi_data[1], midi_data[2], midi_data[3]);
		break;
		case PROGRAM_CHANGE:
			// Read data
			if(reader->read_track_byte(&read_byte) < 1) {
				*current_state = DONE;
				return;
			}

			midi_data[0] = 0x0C;
			midi_data[1] = 0xC0;
			midi_data[2] = read_byte; // Program number
			midi_data[3] = 0;

			printf("Wrote [Program Change]: %02x %02x %02x %02x\n", midi_data[0], midi_data[1], midi_data[2], midi_data[3]);

		break;
		default:
			printf("Read [CODE] code not recognized: %02x\n", code);

			// Length = variable
			if(reader->read_track_byte(&read_byte) < 1) {
				*current_state = DONE;
				return;
			}

			// unknown data (byte length)
			text_data = (unsigned char *) malloc(sizeof(unsigned char) * read_byte);
			if(reader->read_track_bytes(text_data, read_byte) < read_byte) {
				*current_state = DONE;
				return;
			}
			printf("Read [UNKNOWN]: %s\n", text_data);
		break;
	}

	midi_command_to_ascii(midi_data, midi_ascii_data);
	udp_stream(midi_ascii_data);
}

void wait_for(unsigned int delta_time) {
	if (delta_time > 0) {
				usleep(delta_time* m_file->bpm*1000/ m_file->time_division);
			}
}

unsigned int get_next_delta_time(state *current_state, ByteReader *reader) {
	unsigned int delta_time = 0;
	unsigned char byte = 0;
	bool running_status;

	if(reader->read_track_byte(&byte) < 1) {
		*current_state = DONE;
		printf("could not read 1\n");
		return delta_time;
	}
	delta_time = delta_time | byte;

	running_status = (byte & 0x80) == 0x80;
	while(running_status) {
		if(reader->read_track_byte(&byte) < 1) {
			printf("could not read 2\n");
			*current_state = DONE;
			return delta_time;
		}
		delta_time = ((delta_time & 0x7F) << 7) | (byte & 0x7F);
		running_status = (byte & 0x80) == 0x80;
	}

	return delta_time;
}

unsigned char get_next_midi_code(state *current_state, ByteReader *reader) {
	unsigned char code;
	bool running_status;

	if(reader->read_track_byte(&code) < 1) {
		*current_state = DONE;
		return code;
	}

	running_status = (code & 0x80) != 0x80;
	if (running_status) {
		reader->unread_track_byte();
	}

	return code;
}

void *play_midi_track_udp(void *track_data)
{
	midi_track *m_track;
	m_track = (midi_track *)track_data;
	ByteReader reader(m_track);

	unsigned short num_tracks;
	bool running_status = false;
	unsigned char code, byte = 0;
	unsigned int delta_time = 0;
	int current_track_num = 0;

	state current_state = START;

	printf("Playing track #%d | size = %02x\n", m_track->track_num, m_track->size);
	printf("TIME SIGNATURE NPQ = %02x DIV = %02x BPM = %02x\n",
			m_file->time_signature.notes_pqn,
			m_file->time_division, m_file->bpm
	);

	// Start track playback
	unsigned char data[100];
	current_state = START;
	wait_for(m_track->delta_time);

	while (current_state != DONE) {
		delta_time = get_next_delta_time(&current_state, &reader);
		code = get_next_midi_code(&current_state, &reader);
		decode_next_state(&current_state, code, &reader);
		wait_for(delta_time);
	}

	return 0;
}

int parseFile(const char *filename, midi_file **m_file) {
	unsigned int riff_len, delta_time;
	unsigned short current_track_num;
	unsigned char byte, data[4];
	midi_track *m_tracks;
	ByteReader reader(filename);
	*m_file = (midi_file *)malloc(sizeof(midi_file));

	// Parse header
	// RIFF Chunk MThd
	if(reader.read_bytes(data, 4) < 4) {
		printf("Not a midi file\n");
		return -1;
	}
	if (memcmp(data, "MThd", 4) != 0) {
		printf("Not a midi file\n");
		return -1;
	}
	printf("got: %02x %02x %02x %02x\n",data[0],data[1],data[2],data[3]);

	// RIFF Chunk length
	if(reader.read_bytes(data, 4) < 4) {
		printf("Corrupt midi file 1\n");
		return -1;
	}
	printf("got: %02x %02x %02x %02x\n",data[0],data[1],data[2],data[3]);
	riff_len = ((((((0 | data[0]) << 8) | data[1]) << 8) | data[2]) << 8) | data[3];
	if (riff_len != 6) {
		printf("Corrupt midi file. Expect length of 6 but got %d\n", riff_len);
		return -1;
	}

	// Midi format
	if(reader.read_bytes(data, 2) < 2) {
		printf("Corrupt midi file 3\n");
		return -1;
	}

	(*m_file)->format = ((0 | data[0]) << 8) | data[1];
	printf("got [FROMAT]: %02x %02x = %d\n",data[0],data[1], (*m_file)->format);

	// Midi number of tracks
	if(reader.read_bytes(data, 2) < 2) {
		printf("Corrupt midi file 4\n");
		return -1;
	}
	(*m_file)->num_tracks = ((0 | data[0]) << 8) | data[1];
	printf("got: %02x %02x\n",data[0],data[1]);

	// Midi time division
	if(reader.read_bytes(data, 2) < 2) {
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
			if(reader.read_byte(&byte) < 1)
				break;
			delta_time = ((delta_time & 0x7F) << 7) | (byte & 0x7F);
		}


		m_tracks[current_track_num].delta_time = delta_time;
		printf("Parsing Track #%d\n", current_track_num + 1);

		// RIFF Chunk MTrk
		if(reader.read_bytes(data, 4) < 4) {
			printf("Corrupt midi file 6\n");
			return -1;
		}
		printf("MTrk: %02x %02x %02x %02x\n", data[0], data[1], data[2], data[3]);
		if (memcmp(data, "MTrk", 4) != 0) {
			printf("No tracks found\n");
			return -1;
		}

		// RIFF Chunk length
		if(reader.read_bytes(data, 4) < 4) {
			printf("Corrupt midi file 7\n");
			return -1;
		}
		printf("Reading next: %02x %02x %02x %02x bytes for data\n", data[0], data[1], data[2], data[3]);
		riff_len = (((((((0 | data[0]) << 8) | data[1]) << 8) | data[2]) << 8) | data[3]) - 1;

		m_tracks[current_track_num].size = riff_len;
		m_tracks[current_track_num].track_num = current_track_num + 1;
		m_tracks[current_track_num].data = (unsigned char *) malloc(sizeof(unsigned char)*riff_len);

		if(reader.read_bytes(m_tracks[current_track_num].data, riff_len) < riff_len) {
			printf("Error track terminated before %d bytes.\n", riff_len);
			return -1;
		}
		printf("READ: %02x %02x %02x %02x bytes for data\n",
						data[0],data[1],data[2],data[3]);
		printf("m_tracks[%d].data (size=%02x) == data!\n",current_track_num,riff_len);
	} while (reader.read_byte(&byte) == 1 && ++current_track_num < (*m_file)->num_tracks);

	(*m_file)->tracks = m_tracks;

	printf("done\n");
	return 1;
}

void hold_pedal(bool down_state) {
	unsigned char pedal_ascii_data[100];
	unsigned char pedal_midi_command[4];

	pedal_midi_command[0] = 0x0B;
	pedal_midi_command[1] = 0xB0;
	pedal_midi_command[2] = 0x40;

	if(down_state) {
		// Hold down pedals
		pedal_midi_command[3] = 0x7F;
	} else {
		// Lift up pedals
		pedal_midi_command[3] = 0x00;
	}

	midi_command_to_ascii(pedal_midi_command, pedal_ascii_data);
	udp_stream(pedal_ascii_data);
}

void playMidi_udp()
{
	if(m_file->s == -1)
		return;

	hold_pedal(true);
	usleep(1000*2000);

	printf("Playing Midi...\n");

	pthread_t *threads;
	int *t_rets, i;

	threads = (pthread_t *)malloc(sizeof(pthread_t)* m_file->num_tracks);
	t_rets = (int *)malloc(sizeof(int)* m_file->num_tracks);

	m_file->bpm = 1200;

	// Create independent threads each of which will execute function
	for(i=0; i< m_file->num_tracks; ++i)
		t_rets[i] = pthread_create( &threads[i], NULL, play_midi_track_udp, &(m_file->tracks[i]));

	// Create independent threads each of which will execute function
	for(i=0; i< m_file->num_tracks; ++i) {
		// Wait till threads are complete before main continues.
		pthread_join( threads[i], NULL);
		printf("Waiting for track #%d.\n", i+1);
	}

	usleep(1000*2000);
	hold_pedal(false);
	printf("Midi playback complete.\n");
}

// Using the custom usb driver,
// This main will send the signals
// over to a UDP server.
#define BUFLEN 512
#define PORT 4444

int main(void)
{
	struct sockaddr_in si_me, si_other;
	int s, i, slen=sizeof(si_other);
	char buf[BUFLEN];

	struct pollfd pfds[1];
	int fd, avail_data;
	unsigned char buf2[1024], data3[100];

	if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
		printf("Could not create socket\n");
	else
		printf("Successfully created socket.\n");

	memset((char *) &si_me, 0, sizeof(si_me));
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(PORT);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(s, (sockaddr *)&si_me, sizeof(si_me))==-1)
		printf("Could not bind port\n");
	else
		printf("Successfully binded to port.\n");

	while(1) {

		if(recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *)&si_other, (socklen_t *)&slen)==-1)
			printf("received nothing from the client...\n");
		printf("Received packet from %s:%d\nData: %s\n\n",
				inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port), buf);

		if(strcmp(buf,"START") != 0)
			continue;

		clock_t start_t;
		double elapsedTime = 0;

		start_t = clock();

		parseFile("/Users/haleeq/Downloads/cl.mid", &m_file);
		m_file->s = s;
		m_file->si_other = si_other;
		m_file->slen = slen;
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

		sendto(s, data3, 4, 0, (sockaddr *)&si_other, slen);

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
					sendto(s, buf2, 4, 0, (sockaddr *)&si_other, slen);
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
}
