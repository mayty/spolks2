#ifndef SHARED_ICMP
#define SHARED_ICMP

#include <inttypes.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip.h>

void send_echo(int fd, struct sockaddr_in dest, uint16_t seq, const char* payload, size_t payload_size);

typedef struct {
    enum {
        ECHOREPLY_OK,
        ECHOREPLY_TTL_EXCEEDED,
        ECHOREPLY_NO_REPLY,
        ECHOREPLY_WRONG_PAYLOAD,
    } status;

    double delay;
    struct sockaddr_in responder;
} EchoReplyResult;

EchoReplyResult recv_echo_reply(
    int                fd,
    struct sockaddr_in dest,
    double             timeout,
    uint16_t           seq,
    const char*        orig_payload,
    size_t             orig_payload_size
);

// TCP  - 5 words
// ICMP - 2 words
// word - 4 bytes
// can't use sizeof(struct ...) because of alignment padding
#define HEADERS_LEN ((5 + 2) * 4)

void build_fake_header(void* buffer, struct sockaddr_in addr, uint8_t subnet, uint16_t total_length);
void write_checksum(void* buffer, size_t len);

#endif
