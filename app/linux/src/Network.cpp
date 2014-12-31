#include "include/global.h"
#include "include/Midi.h"
#include "include/Network.h"
#include "include/MidiTrackDecoder.h"

Network::Network(Midi *midi_info) {
    midi = midi_info;
}

void Network::send_udp_packet(unsigned char *midi_ascii_data) {
    sendto(
            midi->file->net_socket,
            midi_ascii_data,
            strlen((char *)midi_ascii_data),
            0,
            (sockaddr *)&(midi->file->remote_net_socket_in),
            midi->file->net_socket_len
    );
}

void Network::hold_pedal(bool down_state) {
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

    midi->midi_command_to_ascii(pedal_midi_command, pedal_ascii_data);
    send_udp_packet(pedal_ascii_data);
}

void Network::playMidi_udp()
{
    if(midi->file->net_socket == -1)
        return;

    hold_pedal(true);
    usleep(1000*2000);

    printf("Playing Midi...\n");

    pthread_t *threads;
    int *t_rets, i;

    threads = (pthread_t *)malloc(sizeof(pthread_t)* midi->file->num_tracks);
    t_rets = (int *)malloc(sizeof(int)* midi->file->num_tracks);

    // Create independent threads each of which will execute function
    for(i=0; i< midi->file->num_tracks; ++i) {
        midi_track_thread_args *thread_args = (midi_track_thread_args *)malloc(sizeof(midi_track_thread_args));
        thread_args->instance = this;
        thread_args->track_num = i;

        t_rets[i] = pthread_create(&threads[i], NULL, play_midi_track_udp_ts, thread_args);
        midi->wait_for(midi->file->tracks[i].delta_time);
    }

    // Create independent threads each of which will execute function
    for(i=0; i< midi->file->num_tracks; ++i) {
        // Wait till threads are complete before main continues.
        pthread_join( threads[i], NULL);
        printf("Waiting for track #%d.\n", i+1);
    }

    usleep(1000*2000);
    hold_pedal(false);
    printf("Midi playback complete.\n");
}

void *Network::play_midi_track_udp(int track_num)
{
    midi_track *m_track;
    m_track = &midi->file->tracks[track_num];
    MidiTrackDecoder *midi_track_decoder = new MidiTrackDecoder(midi->file, m_track);

    unsigned int delta_time = 0;
    unsigned char *midi_data;
    unsigned char midi_ascii_data[100];

    printf("Playing track #%d | size = %02x\n", m_track->track_num, m_track->size);

    // Start track playback
    midi->wait_for(m_track->delta_time);
    while (midi_track_decoder->get_state() != DONE) {
        delta_time = midi_track_decoder->get_next_delta_time();
        midi_data = midi_track_decoder->decode_next_state();
        midi->midi_command_to_ascii(midi_data, midi_ascii_data);
        send_udp_packet(midi_ascii_data);
        midi->wait_for(delta_time);
    }

    return 0;
}

void *Network::play_midi_track_udp_ts(void *thread_args) {
    midi_track_thread_args *args = (midi_track_thread_args *)thread_args;
    args->instance->play_midi_track_udp(args->track_num);

    return 0;
}

