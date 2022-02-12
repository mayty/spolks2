#include "network.h"

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/time.h>

int get_addr(struct sockaddr_in* addr, const char* str_addr) {
    addr->sin_family = AF_INET;

    int result = inet_aton(str_addr, &addr->sin_addr);
    if (result == 0) {
        return 1;
    }

    return 0;
}

void set_ttl(int fd, uint8_t value) {
    int result = setsockopt(fd, IPPROTO_IP, IP_TTL, &value, sizeof(value));
    if (result < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
}

void reset_ttl(int fd) {
    set_ttl(fd, IPDEFTTL);
}

void set_timeout(int fd, unsigned int seconds) {
    struct timeval value;
    value.tv_sec = seconds;
    value.tv_usec = 0;

    int result = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*) &value, sizeof(value));
    if (result < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
}

void unset_timeout(int fd) {
    set_timeout(fd, 0);
}

void toggle_recverr(int fd, int state) {
    int value = state ? 1 : 0;

    int res = setsockopt(fd, SOL_IP, IP_RECVERR, (void*) &value, sizeof(value));
    if (res < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
}

void toggle_broadcast(int fd, int state) {
    int value = state ? 1 : 0;

    int result = setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &value, sizeof(value));
    if (result < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
}

static const struct sockaddr_in BROADCAST = {
    AF_INET,
    0,
    {
        INADDR_BROADCAST,
    },
};

int send_broadcast(int fd, void* buffer, size_t size) {
    int result = sendto(fd, buffer, size, 0, (const struct sockaddr*) &BROADCAST, sizeof(BROADCAST));
    return result;
}
