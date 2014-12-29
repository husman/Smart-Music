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
#include "include/MidiTrackDecoder.h"
#include "include/MidiFileDecoder.h"

midi_file *m_file;

void udp_stream(unsigned char *midi_ascii_data) {
	sendto(m_file->net_socket, midi_ascii_data, strlen((char *)midi_ascii_data), 0, (sockaddr *)&(m_file->remote_net_socket_in), m_file->net_socket_len);
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

void wait_for(unsigned int delta_time) {
	if (delta_time > 0) {
				usleep(delta_time* m_file->bpm*1000/ m_file->time_division);
			}
}

void *play_midi_track_udp(void *track_data)
{
	midi_track *m_track;
	m_track = (midi_track *)track_data;
	MidiTrackDecoder *midi_track_decoder = new MidiTrackDecoder(m_file, m_track);

	unsigned int delta_time = 0;
	unsigned char *midi_data;
	unsigned char midi_ascii_data[100];

	printf("Playing track #%d | size = %02x\n", m_track->track_num, m_track->size);

	// Start track playback
	wait_for(m_track->delta_time);
	while (midi_track_decoder->get_state() != DONE) {
		delta_time = midi_track_decoder->get_next_delta_time();
		midi_data = midi_track_decoder->decode_next_state();
		midi_command_to_ascii(midi_data, midi_ascii_data);
		udp_stream(midi_ascii_data);
		wait_for(delta_time);
	}

	return 0;
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
	if(m_file->net_socket == -1)
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
	struct sockaddr_in local_net_socket_in, remote_net_socket_in;
	int net_socket;
	char received_packet[BUFLEN];
	socklen_t net_socket_len = sizeof(remote_net_socket_in);

	if ((net_socket =socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1) {
		printf("Could not create socket\n");
	} else {
		printf("Successfully created socket.\n");
	}

	memset((char *) &local_net_socket_in, 0, sizeof(local_net_socket_in));
	local_net_socket_in.sin_family = AF_INET;
	local_net_socket_in.sin_port = htons(PORT);
	local_net_socket_in.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(net_socket, (sockaddr *)&local_net_socket_in, sizeof(local_net_socket_in))==-1) {
		printf("Could not bind port\n");
	} else {
		printf("Successfully binded to port.\n");
	}

	while(recvfrom(net_socket, received_packet, BUFLEN, 0, (struct sockaddr *)&remote_net_socket_in, &net_socket_len) != -1) {
		printf("Received packet from %s:%d\nData: %s\n\n",
				inet_ntoa(remote_net_socket_in.sin_addr),
				ntohs(remote_net_socket_in.sin_port),
				received_packet
		);

		if(strcmp(received_packet, "START") != 0) {
			continue;
		}

		clock_t start_t;
		double elapsedTime = 0;
		start_t = clock();

		midi_file file;
		MidiFileDecoder *midi_file_decoder = new MidiFileDecoder(&file, "/Users/haleeq/Downloads/rfy.mid");
		file.tracks = midi_file_decoder->get_midi_tracks();
		file.net_socket = net_socket;
		file.remote_net_socket_in = remote_net_socket_in;
		file.net_socket_len = net_socket_len;
		m_file = &file;

		playMidi_udp();

		elapsedTime = (double) (clock() - start_t) / CLOCKS_PER_SEC;
		printf("The song went on for: %f seconds = %f minutes\n", elapsedTime, elapsedTime / 60);
	}
}
