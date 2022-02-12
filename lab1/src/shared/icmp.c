#include "icmp.h"
#include "clock.h"

#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/errqueue.h>
#include <netinet/in.h>
#include <sys/socket.h>

void send_echo(int fd, struct sockaddr_in dest, uint16_t seq, const char* payload, size_t payload_size) {
    struct icmphdr packet;
    packet.type = ICMP_ECHO;
    packet.code = 0;
    packet.un.echo.id = 0;
    packet.un.echo.sequence = seq;

    char buffer[sizeof(struct icmphdr) + payload_size];
    memcpy(buffer, &packet, sizeof(packet));
    memcpy(buffer + sizeof(packet), payload, payload_size);

    int sent = sendto(fd, buffer, sizeof(buffer), 0, (void*) &dest, sizeof(dest));
    if (sent <= 0) {
        perror("sendto");
        exit(EXIT_FAILURE);
    }
}

EchoReplyResult recv_echo_reply(int fd, struct sockaddr_in dest, double timeout, uint16_t seq, const char* orig_payload, size_t orig_payload_size) {
    char buffer[sizeof(struct icmphdr) + orig_payload_size];

    struct sockaddr_in src;
    socklen_t src_len = sizeof(src);

    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN;

    time_point start, now;
    get_time_point(&start);

    EchoReplyResult reply_result;

    while (1) {
        get_time_point(&now);

        double diff = get_time_diff(&start, &now);
        int ms_left = (timeout - diff) * 1000;

        if (ms_left <= 0) {
            reply_result.status = ECHOREPLY_NO_REPLY;
            break;
        }

        pfd.revents = 0;

        int result = poll(&pfd, 1, ms_left);
        if (result < 0) {
            perror("poll");
            exit(EXIT_FAILURE);
        }

        if (result == 0) {
            reply_result.status = ECHOREPLY_NO_REPLY;
            break;
        }

        int error = pfd.revents & POLLERR;

        struct msghdr msg;

        msg.msg_name = &src;
        msg.msg_namelen = sizeof(src);

        struct iovec datavec[1];
        datavec[0].iov_base = (void*) buffer;
        datavec[0].iov_len = sizeof(buffer);
        msg.msg_iov = datavec;
        msg.msg_iovlen = 1;

        union {
            struct cmsghdr hdr;
            char additional[sizeof(struct cmsghdr) + 1024];
        } cmsg;
        msg.msg_control = &cmsg;
        msg.msg_controllen = sizeof(cmsg);

        msg.msg_flags = 0;

        int received = recvmsg(fd, &msg, error ? MSG_ERRQUEUE : 0);
        if (received < 0) {
            perror("recvmsg");
            exit(EXIT_FAILURE);
        }

        if (error) {
            struct sock_extended_err* err = NULL;
            for (struct cmsghdr* hdr = CMSG_FIRSTHDR(&msg); hdr != NULL; hdr = CMSG_NXTHDR(&msg, hdr)) {
                if (hdr->cmsg_level == SOL_IP && hdr->cmsg_type == IP_RECVERR) {
                    err = (void*) CMSG_DATA(hdr);
                }
            }

            if (
                err == NULL ||
                err->ee_origin != SO_EE_ORIGIN_ICMP ||
                err->ee_type != ICMP_TIME_EXCEEDED ||
                err->ee_code != 0
            ) {
                continue;
            }

            struct sockaddr* err_addr = SO_EE_OFFENDER(err);

            reply_result.status = ECHOREPLY_TTL_EXCEEDED;
            memcpy(&reply_result.responder, err_addr, sizeof(reply_result.responder));
            break;
        }

        if (received < sizeof(struct icmphdr)) {
            continue;
        }

        if ( // check if packet originated by `dest` host
            src_len != sizeof(struct sockaddr_in) ||
            src.sin_family != AF_INET ||
            src.sin_addr.s_addr != dest.sin_addr.s_addr
        ) {
            continue;
        }

        const struct icmphdr* icmp = (void*) buffer;
        const void* data = buffer + sizeof(struct icmphdr);
        const size_t data_len = received - sizeof(struct icmphdr);

        if (icmp->type != ICMP_ECHOREPLY || icmp->code != 0) {
            continue;
        }

        if (icmp->un.echo.sequence != seq) {
            continue;
        }

        if (
            data_len != orig_payload_size ||
            memcmp(data, orig_payload, orig_payload_size) != 0
        ) {
            reply_result.status = ECHOREPLY_WRONG_PAYLOAD;
            break;
        }

        reply_result.status = ECHOREPLY_OK;
        memcpy(&reply_result.responder, &src, sizeof(reply_result.responder));
        break;
    }

    get_time_point(&now);
    reply_result.delay = get_time_diff(&start, &now);

    return reply_result;
}

static uint32_t broadcast_mask(uint8_t subnet) {
    int positions = 32 - subnet;
    if (positions < 0) {
        positions = 0;
    }

    uint32_t mask = 0;
    for (int i = 0; i < positions; i++) {
        mask |= (1 << i);
    }

    uint32_t network_mask = htonl(mask);
    return network_mask;
}

void build_fake_header(void* buffer, struct sockaddr_in addr, uint8_t subnet, uint16_t total_length) {
    struct iphdr* ip = buffer;
    ip->version = 4;
    ip->ihl = 5; // no options
    ip->tos = 0;
    ip->tot_len = total_length;
    ip->id = htons(1);
    ip->frag_off = 0;
    ip->ttl = 64;
    ip->protocol = IPPROTO_ICMP;
    ip->check = 0;
    ip->saddr = addr.sin_addr.s_addr;
    ip->daddr = addr.sin_addr.s_addr | broadcast_mask(subnet);

    struct icmphdr* icmp = (void*) (ip + 1);
    icmp->type = ICMP_ECHO;
    icmp->code = 0;
    icmp->checksum = 0;
    icmp->un.echo.id = 0;
    icmp->un.echo.sequence = 0;
}

static uint16_t check_sum(const void* ptr, size_t len) {
    const uint16_t* data = ptr;
    uint32_t sum = 0;

    int words = len / 2;
    for (int i = 0; i < words; i++) {
        sum += data[i];
    }

    if (len % 2) {
        sum += ((char*) ptr)[len-1];
    }

    sum = (sum & 0xFFFF) + ((sum & 0xFFFF0000) >> 16);
    sum = ~sum;
    return sum;
}

void write_checksum(void* buffer, size_t len) {
    void* icmp_buffer = buffer + sizeof(struct iphdr);
    size_t icmp_len = len - sizeof(struct iphdr);

    uint16_t sum = check_sum(icmp_buffer, icmp_len);

    struct iphdr* ip = buffer;
    struct icmphdr* icmp = (void*) (ip + 1);
    icmp->checksum = sum;
}
