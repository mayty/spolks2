#include "traceroute.h"
#include <shared/icmp.h>
#include <shared/network.h>
#include <shared/random.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define MAX_HOPS 30
#define MAX_IP_LENGTH 15 // xxx.xxx.xxx.xxx

void traceroute(struct sockaddr_in dest) {
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
    if (fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    toggle_recverr(fd, 1);

    const int PAYLOAD_SIZE = 16;
    char payload[PAYLOAD_SIZE];
    random_str(payload, PAYLOAD_SIZE, CHARSET_ANY);

    for (int hop = 1; hop <= MAX_HOPS; hop++) {
        set_ttl(fd, hop);
        send_echo(fd, dest, 0, payload, PAYLOAD_SIZE);
        reset_ttl(fd);

        EchoReplyResult result = recv_echo_reply(fd, dest, 1, 0, payload, PAYLOAD_SIZE);

        int hop_ident = ceil(log10(MAX_HOPS));
        printf("%*d ", hop_ident, hop);

        switch (result.status) {
            case ECHOREPLY_OK:
            case ECHOREPLY_TTL_EXCEEDED:
                printf("%-*s %.1lf ms\n", MAX_IP_LENGTH + 1, inet_ntoa(result.responder.sin_addr), result.delay * 1000);
                break;
            default:
                printf("-\n");
                break;
        }

        if (result.status == ECHOREPLY_OK) {
            break;
        }
    }

    close(fd);
}
