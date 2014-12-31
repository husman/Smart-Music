#ifndef NETWORK_H
#define NETWORK_H

class Network {
public:
    static void send_udp_packet(unsigned char *midi_ascii_data, midi_file *m_file);
};

#endif