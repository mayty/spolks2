#ifndef SMURF_SMURF
#define SMURF_SMURF

#include <netinet/in.h>

void smurf(struct sockaddr_in dest, uint8_t subnet, size_t count, size_t payload_size);

#endif
