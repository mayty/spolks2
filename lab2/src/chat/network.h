#pragma once

#include <cstdlib>
#include <functional>
#include <list>
#include <optional>
#include <pthread.h>
#include <netinet/in.h>

struct IfInfo {
    in_addr_t addr;
    in_addr_t mask;
    in_addr_t broadcast;
};

std::optional<IfInfo> getIfInfo();

class MultiSocket {
public:
    using ReceiveCallback = std::function<void (in_addr_t sender, const char* data, size_t size)>;

    const in_addr_t NO_GROUP = INADDR_BROADCAST;

    MultiSocket(unsigned short port);
    virtual ~MultiSocket();

    void send(const void* data, size_t size, bool force_broadcast = false);
    void listen() const;
    void setOnReceive(ReceiveCallback callback);

    void join(in_addr_t group);
    void leave();

    void mute(in_addr_t addr);
    void unmute(in_addr_t addr);
    const std::list<in_addr_t>& getMuted() const;

protected:
    unsigned short port;

    int fd;

    ReceiveCallback recv_callback = nullptr;

    in_addr_t current_group = NO_GROUP;

    std::list<in_addr_t> muted_addr;
};

class MultiSocketListener {
public:
    MultiSocketListener(const MultiSocket& multisocket);
    virtual ~MultiSocketListener();

    void stop();

protected:
    const MultiSocket& multisocket;

    pthread_t handle;

    static void* worker(void* obj_ptr);
};
