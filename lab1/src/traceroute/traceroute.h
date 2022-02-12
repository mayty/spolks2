#ifndef TRACEROUTE_TRACEROUTE
#define TRACEROUTE_TRACEROUTE

#include <netinet/in.h>

void traceroute(struct sockaddr_in dest);

#endif
