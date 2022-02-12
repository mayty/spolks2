#ifndef SHARED_NETWORK
#define SHARED_NETWORK

#include <inttypes.h>
#include <netinet/in.h>

int get_addr(struct sockaddr_in* addr, const char* str_addr);

void set_ttl(int fd, uint8_t value);
void reset_ttl(int fd);

void set_timeout(int fd, unsigned int seconds);
void unset_timeout(int fd);

void toggle_recverr(int fd, int state);

void toggle_broadcast(int fd, int state);

int send_broadcast(int fd, void* buffer, size_t size);

#endif
