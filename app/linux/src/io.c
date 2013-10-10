/*
 ============================================================================
 Name        : io.c
 Author      : Haleeq Usman
 Version     : 0.0
 Description : Smart Music Application (dev)
 ============================================================================
 */
#include "include/global.h"

static const int BUFFER_MAX_CAPACITY = 1000;

int read_byte(unsigned char *dest)
{
	if(midi_fp == NULL || buffer == NULL)
		return -1;

	if(midi_fpos >= buffer_len) {
		memcpy(buffer, buffer + BUFFER_MAX_CAPACITY, BUFFER_MAX_CAPACITY);
		buffer_len = fread(buffer + BUFFER_MAX_CAPACITY, 1, BUFFER_MAX_CAPACITY, midi_fp) + BUFFER_MAX_CAPACITY;
		if(buffer_len <= BUFFER_MAX_CAPACITY)
			return -1;
		midi_fpos = BUFFER_MAX_CAPACITY;
	}

	*dest = buffer[midi_fpos++];

	return 1;
}

int unread_byte()
{
	if(midi_fp == NULL || buffer == NULL)
		return 0;

	if(midi_fpos > 0) {
		--midi_fpos;
		return 1;
	}

	return 0;
}

int read_bytes(unsigned char *dest, int num_bytes)
{
	if(midi_fp == NULL || buffer == NULL)
		return -1;

	int len;
	if(midi_fpos >= buffer_len) {
		memcpy(buffer, buffer + BUFFER_MAX_CAPACITY, BUFFER_MAX_CAPACITY);
		buffer_len = fread(buffer + BUFFER_MAX_CAPACITY, 1, BUFFER_MAX_CAPACITY, midi_fp) + BUFFER_MAX_CAPACITY;

		if(buffer_len <= BUFFER_MAX_CAPACITY)
			return -1;
		midi_fpos = BUFFER_MAX_CAPACITY;

		len = buffer_len - BUFFER_MAX_CAPACITY;
		if(num_bytes > len) {
			memcpy(dest, buffer + midi_fpos, len);

			midi_fpos += len;
			return len;
		} else {
			memcpy(dest, buffer + midi_fpos, num_bytes);

			midi_fpos += num_bytes;
			return num_bytes;
		}
	} else if(buffer_len - midi_fpos < num_bytes) {
		memcpy(dest, buffer + midi_fpos, buffer_len - midi_fpos);

		int bytes_remaining = num_bytes - (buffer_len - midi_fpos);
		int offset = buffer_len - midi_fpos;

		memcpy(buffer, buffer + BUFFER_MAX_CAPACITY, BUFFER_MAX_CAPACITY);
		buffer_len = fread(buffer + BUFFER_MAX_CAPACITY, 1, BUFFER_MAX_CAPACITY, midi_fp) + BUFFER_MAX_CAPACITY;
		if(buffer_len <= BUFFER_MAX_CAPACITY)
			return -1;
		midi_fpos = BUFFER_MAX_CAPACITY;

		if(bytes_remaining > buffer_len - BUFFER_MAX_CAPACITY)
			bytes_remaining = buffer_len - BUFFER_MAX_CAPACITY;

		memcpy(dest + offset, buffer + midi_fpos, bytes_remaining);

		midi_fpos += bytes_remaining;
		return offset + bytes_remaining;
	} else {
		memcpy(dest, buffer + midi_fpos, num_bytes);

		midi_fpos += num_bytes;
	}

	return num_bytes;
}

int unread_bytes(int num_bytes)
{
	if(midi_fp == NULL || buffer == NULL)
		return 0;

	if(num_bytes > midi_fpos)
		num_bytes = midi_fpos;

	if(midi_fpos > 0) {
		midi_fpos -= num_bytes;
		return num_bytes;
	}

	return 0;
}

/* track data readers */
int read_track_byte(unsigned int *track_dpos, midi_track *m_track, unsigned char *dest)
{
	if(m_track == NULL || m_track->data == NULL)
		return 0;

	if(*track_dpos >= m_track->size)
		return 0;

	memcpy(dest, m_track->data + (*track_dpos)++, 1);
	//printf("at: %d; read_track_byte: %02x\n", *track_dpos, *dest);

	return 1;
}

int unread_track_byte(unsigned int *track_dpos, midi_track *m_track)
{
	if(m_track == NULL || m_track->data == NULL)
		return 0;

	if(*track_dpos > 0) {
		--(*track_dpos);
		return 1;
	}

	return 0;
}

int read_track_bytes(unsigned int *track_dpos, midi_track *m_track, unsigned char *dest, int num_bytes)
{
	if(m_track == NULL || m_track->data == NULL)
		return -1;

	if(*track_dpos >= m_track->size)
		return 0;
	else if(m_track->size - *track_dpos < num_bytes) {
		memcpy(dest, m_track->data + *track_dpos, m_track->size - *track_dpos);
		*track_dpos += m_track->size - *track_dpos;
		return m_track->size - *track_dpos;
	} else
		memcpy(dest, m_track->data + *track_dpos, num_bytes);

	*track_dpos += num_bytes;

	return num_bytes;
}

int unread_track_bytes(unsigned int *track_dpos, midi_track *m_track, int num_bytes)
{
	if(m_track == NULL || m_track->data == NULL)
		return 0;

	if(num_bytes > m_track->size - *track_dpos)
		num_bytes = m_track->size - *track_dpos;

	if(*track_dpos > 0) {
		*track_dpos -= num_bytes;
		return num_bytes;
	}

	return 0;
}


void deinitialize()
{
	free(buffer);

	if(midi_fp != NULL)
		fclose(midi_fp);
}

int initialize(const char *filename)
{
	deinitialize();

	buffer = (unsigned char *)malloc(sizeof(unsigned char *)*BUFFER_MAX_CAPACITY*2);
	midi_fpos = BUFFER_MAX_CAPACITY*2;
	buffer_len = midi_fpos;

	midi_fp = fopen(filename, "rb");

	if(midi_fpos >= buffer_len) {
		buffer_len = fread(buffer + BUFFER_MAX_CAPACITY, 1, BUFFER_MAX_CAPACITY, midi_fp) + BUFFER_MAX_CAPACITY;
		memcpy(buffer, buffer + BUFFER_MAX_CAPACITY, BUFFER_MAX_CAPACITY);
		if(buffer_len <= BUFFER_MAX_CAPACITY)
			return -1;
		midi_fpos = BUFFER_MAX_CAPACITY;
	}

	return !buffer? -1 : 1;
}

int reinitialize()
{
	free(buffer);
	buffer = (unsigned char *)malloc(sizeof(unsigned char *)*BUFFER_MAX_CAPACITY*2);
	midi_fpos = BUFFER_MAX_CAPACITY*2;
	buffer_len = midi_fpos;

	if(midi_fpos >= buffer_len) {
		buffer_len = fread(buffer + BUFFER_MAX_CAPACITY, 1, BUFFER_MAX_CAPACITY, midi_fp) + BUFFER_MAX_CAPACITY;
		memcpy(buffer, buffer + BUFFER_MAX_CAPACITY, BUFFER_MAX_CAPACITY);
		if(buffer_len <= BUFFER_MAX_CAPACITY)
			return -1;
		midi_fpos = BUFFER_MAX_CAPACITY;
	}

	return !buffer? -1 : 1;
}
unsigned int skip_bytes(unsigned int num_bytes)
{
	if(midi_fp == NULL || buffer == NULL)
		return 0;

	unsigned int bytes_remaining = num_bytes, read_amt;
	unsigned char *bytes;

	printf("value =  %d\n", (buffer_len - midi_fpos));
	if(num_bytes > (buffer_len - midi_fpos)) {
		bytes_remaining -= buffer_len - midi_fpos;
		midi_fpos = buffer_len;
		printf("bytes_remaining =  %d\n", bytes_remaining);
		if(bytes_remaining > 0) {
			bytes = (unsigned char*)malloc(sizeof(unsigned char*)*bytes_remaining);
			read_amt = fread(bytes, 1, bytes_remaining, midi_fp);
			free(bytes);
			bytes_remaining -= read_amt;
			printf("bytes_remaining =  %d\n", bytes_remaining);
		}
		return num_bytes - bytes_remaining;
	}

	midi_fpos += num_bytes;

	return num_bytes;
}
