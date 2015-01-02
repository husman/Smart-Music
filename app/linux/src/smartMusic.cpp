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
#include "include/global.h"
#include "include/Network.h"
#include "include/MidiFileDecoder.h"
#include "include/Midi.h"


midi_file *m_file;

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
		MidiFileDecoder *midi_file_decoder = new MidiFileDecoder(&file, "/Users/haleeq/Downloads/midi.mid");
		file.tracks = midi_file_decoder->get_midi_tracks();
		file.net_socket = net_socket;
		file.remote_net_socket_in = remote_net_socket_in;
		file.net_socket_len = net_socket_len;
		m_file = &file;

		Midi *midi = new Midi(&file);

		Network *network = new Network(midi);
		network->playMidi_udp();

		elapsedTime = (double) (clock() - start_t) / CLOCKS_PER_SEC;
		printf("The song went on for: %f seconds = %f minutes\n", elapsedTime, elapsedTime / 60);
	}
}
