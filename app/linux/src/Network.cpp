#include "include/global.h"
#include "include/Network.h"

void Network::send_udp_packet(unsigned char *midi_ascii_data, midi_file *m_file) {
    sendto(
            m_file->net_socket,
            midi_ascii_data,
            strlen((char *)midi_ascii_data),
            0,
            (sockaddr *)&(m_file->remote_net_socket_in),
            m_file->net_socket_len
    );
}