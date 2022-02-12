#include "ping.h"
#include <shared/clock.h>
#include <shared/icmp.h>
#include <shared/random.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

void ping(struct sockaddr_in dest, unsigned short attempts, double timeout, u_int16_t payload_bytes) {
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
    if (fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    for (int seq = 0; attempts ? (seq < attempts) : 1; seq++) {
        char payload[payload_bytes];
        random_str(payload, payload_bytes, CHARSET_ANY);

        send_echo(fd, dest, seq, payload, sizeof(payload));

        EchoReplyResult result = recv_echo_reply(fd, dest, timeout, seq, payload, sizeof(payload));
        switch (result.status) {
            case ECHOREPLY_OK:
                printf("%s\t%.1lfms\n", inet_ntoa(dest.sin_addr), result.delay * 1000);
                break;
            default:
                printf("%s\t-\n", inet_ntoa(dest.sin_addr));
                break;
        }

        double wait_time = timeout - result.delay;
        if (wait_time > 0 && seq != attempts - 1) {
            struct timespec ts = lftots(wait_time);
            nanosleep(&ts, NULL);
        }
    }

    close(fd);
}
