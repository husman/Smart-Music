/*
 ============================================================================
 Name        : io.c
 Author      : Haleeq Usman
 Version     : 0.0
 Description : Smart Music Application (dev)
 ============================================================================
 */
#include "include/MidiTrackReader.h"

MidiTrackReader::MidiTrackReader(midi_track *track_data) {
	BUFFER_MAX_CAPACITY = 8000;
	m_track = track_data;
}

int MidiTrackReader::read_track_byte(unsigned char *dest)
{
	if (m_track == NULL || m_track->data == NULL)
		return 0;

	if (track_dpos >= m_track->size)
		return 0;

	memcpy(dest, m_track->data + track_dpos++, 1);

	return 1;
}

int MidiTrackReader::unread_track_byte()
{
	if (m_track == NULL || m_track->data == NULL)
		return 0;

	if (track_dpos > 0) {
		--track_dpos;
		return 1;
	}

	return 0;
}

int MidiTrackReader::read_track_bytes(unsigned char *dest, int num_bytes)
{
	if (m_track == NULL || m_track->data == NULL)
		return -1;

	if (track_dpos >= m_track->size)
		return 0;
	else if (m_track->size - track_dpos < num_bytes) {
		memcpy(dest, m_track->data + track_dpos, m_track->size - track_dpos);
		track_dpos += m_track->size - track_dpos;
		return m_track->size - track_dpos;
	} else
		memcpy(dest, m_track->data + track_dpos, num_bytes);

	track_dpos += num_bytes;

	return num_bytes;
}

int MidiTrackReader::unread_track_bytes(int num_bytes)
{
	if (m_track == NULL || m_track->data == NULL)
		return 0;

	if (num_bytes > m_track->size - track_dpos)
		num_bytes = m_track->size - track_dpos;

	if (track_dpos > 0) {
		track_dpos -= num_bytes;
		return num_bytes;
	}

	return 0;
}
