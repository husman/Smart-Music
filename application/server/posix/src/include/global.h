/*
 ============================================================================
 Name        : global.h
 Author      : Haleeq Usman
 Version     : 0.0
 Description : Smart Music Application (dev)
 ============================================================================
 */

#ifndef GLOBAL_H_
#define GLOBAL_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>


#include <fcntl.h>
#include <sys/poll.h>
#include <sys/time.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

//#include <alsa/asoundlib.h>

typedef enum {
	START,
	RUNNING,
	DONE
} state;

typedef enum {
	NOTE_ON = 0x90,
	NOTE_OFF = 0x80,
	TEMPO = 0x51,
	META_SYSTEM = 0xF0,
	SET_TEMPO = 0x51,
	CONTROLLER = 0xB0,
	POLY_AFTERTOUCH = 0xA0,
	TEXT = 0x01,
	COPYRIGHT = 0x02,
	TRACK_NAME = 0x03,
	INSTRUMENT_NAME = 0x04,
	LYRICS = 0x05,
	MARKER = 0x06,
	CUE_POINT = 0x07,
	MIDI_CHANNEL_PREFIX = 0x20,
	END_OF_TRACK = 0X2F,
	TIME_SIGNATURE = 0x58,
	KEY_SIGNATURE = 0x59,
	PROGRAM_CHANGE = 0xC0,
	SMPTE_OFFSET = 0x54
} MIDI;

typedef struct time_sig {
	unsigned char numer;
	unsigned char denom;
	unsigned char metro;
	unsigned char notes_pqn;
} time_sig;

typedef struct key_sig {
	unsigned char key;
	unsigned char scale;
} key_sig;

typedef struct midi_track {
	unsigned int delta_time;
	unsigned int size;
	unsigned int track_num;
	unsigned char *data;

} midi_track;

typedef struct midi_file {
	unsigned short format;
	unsigned short num_tracks;
	unsigned short time_division;
	time_sig time_signature;
	key_sig key_signature;
	midi_track *tracks;
	unsigned int bpm;
	FILE *drv_ptr;
	int connfd, net_socket;
	socklen_t net_socket_len;
	struct sockaddr_in remote_net_socket_in;

} midi_file;


int read_byte(unsigned char *dest);
int unread_byte(void);
int read_bytes(unsigned char *dest, int num_bytes);
int unread_bytes(int num_bytes);

/* track data readers */
int read_track_byte(unsigned int *track_dpos, midi_track *m_track, unsigned char *dest);
int unread_track_byte(unsigned int *track_dpos, midi_track *m_track);
int read_track_bytes(unsigned int *track_dpos, midi_track *m_track, unsigned char *dest, int num_bytes);
int unread_track_bytes(unsigned int *track_dpos, midi_track *m_track, int num_bytes);

unsigned int skip_bytes(unsigned int num_bytes);

int initialize(const char *filename);
int reinitialize();
void deinitialize(void);

#endif /* GLOBAL_H_ */
