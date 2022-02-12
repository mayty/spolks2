#ifndef PING_PING
#define PING_PING

#include <netinet/in.h>

void ping(struct sockaddr_in dest, unsigned short attempts, double timeout, u_int16_t payload_bytes);

#endif
