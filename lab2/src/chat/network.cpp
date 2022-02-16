#include "network.h"

#include <algorithm>
#include <cstdio>
#include <ifaddrs.h>
#include <unistd.h>
#include <sys/socket.h>

std::optional<IfInfo> getIfInfo() {
    ifaddrs* ifaces;
    if (getifaddrs(&ifaces) < 0) {
        perror("getifaddrs");
        return {};
    }

    struct ifaddrs *ifaddr = ifaces;
    for (; ifaddr; ifaddr = ifaddr->ifa_next) {
        if (ifaddr->ifa_addr->sa_family != AF_INET) {
            continue;
        }

        sockaddr_in* sockaddr = (sockaddr_in*) ifaddr->ifa_addr;
        in_addr_t addr = sockaddr->sin_addr.s_addr;
        if (ntohl(addr) != INADDR_LOOPBACK) {
            break;
        }
    }

    if (!ifaddr) {
        freeifaddrs(ifaces);
        return {};
    }

    IfInfo result;

    sockaddr_in* addr = (sockaddr_in*) ifaddr->ifa_addr;
    sockaddr_in* mask = (sockaddr_in*) ifaddr->ifa_netmask;
    sockaddr_in* broadcast = (sockaddr_in*) ifaddr->ifa_ifu.ifu_broadaddr;

    result.addr = addr->sin_addr.s_addr;
    result.mask = mask->sin_addr.s_addr;
    result.broadcast = broadcast->sin_addr.s_addr;

    freeifaddrs(ifaces);

    return result;
}

MultiSocket::MultiSocket(unsigned short port) : port(port) {
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    sockaddr_in listen_addr;
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = INADDR_ANY;
    listen_addr.sin_port = htons(port);

    int result = bind(fd, (sockaddr*) &listen_addr, sizeof(listen_addr));
    if (result < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    int broadcast = 1;
    result = setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    if (result < 0) {
        perror("setsockopt(SO_BROADCAST)");
        exit(EXIT_FAILURE);
    }
}

MultiSocket::~MultiSocket() {
    close(fd);
}

void MultiSocket::send(const void* data, size_t size, bool force_broadcast) {
    struct sockaddr_in addr = {
        AF_INET,
        htons(port),
        force_broadcast ? INADDR_BROADCAST : htonl(current_group),
    };
    socklen_t addrlen = sizeof(addr);

    int result = sendto(fd, data, size, 0, (struct sockaddr*) &addr, addrlen);
    if (result < 0) {
        perror("sendto");
        exit(EXIT_FAILURE);
    }
}

void MultiSocket::listen() const {
    char buffer[1024];

    sockaddr src_addr;
    socklen_t src_addrlen = sizeof(src_addr);

    while (1) {
        src_addrlen = sizeof(src_addr);

        int result = recvfrom(fd, buffer, sizeof(buffer), 0, (struct sockaddr*) &src_addr, &src_addrlen);
        if (result < 0) {
            perror("recvfrom");
            exit(EXIT_FAILURE);
        }

        if (src_addrlen != sizeof(sockaddr_in)) {
            continue;
        }

        if (recv_callback) {
            sockaddr_in* src_addr_in = (sockaddr_in*) &src_addr;
            recv_callback(src_addr_in->sin_addr.s_addr, buffer, result);
        }
    }
}

void MultiSocket::setOnReceive(ReceiveCallback callback) {
    recv_callback = callback;
}

void MultiSocket::join(in_addr_t group) {
    leave();

    struct ip_mreq mreq;
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    mreq.imr_multiaddr.s_addr = htonl(group);

    int result = setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
    if (result < 0) {
        perror("setsockopt(IP_ADD_MEMBERSHIP)");
        exit(EXIT_FAILURE);
    }

    current_group = group;
}

void MultiSocket::leave() {
    if (current_group == NO_GROUP) {
        return;
    }

    struct ip_mreq mreq;
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    mreq.imr_multiaddr.s_addr = htonl(current_group);

    int result = setsockopt(fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
    if (result < 0) {
        perror("setsockopt(IP_DROP_MEMBERSHIP)");
        exit(EXIT_FAILURE);
    }

    current_group = NO_GROUP;
}

void MultiSocket::mute(in_addr_t addr) {
    auto it = std::find(muted_addr.begin(), muted_addr.end(), addr);
    if (it != muted_addr.end()) {
        return;
    }

    muted_addr.push_back(addr);

    struct ip_mreq_source source;
    source.imr_interface.s_addr = htonl(INADDR_ANY);
    source.imr_multiaddr.s_addr = htonl(current_group);
    source.imr_sourceaddr.s_addr = addr;

    int result = setsockopt(fd, IPPROTO_IP, IP_BLOCK_SOURCE, &source, sizeof(source));
    if (result < 0) {
        perror("setsockopt(IP_BLOCK_SOURCE)");
        exit(EXIT_FAILURE);
    }
}

void MultiSocket::unmute(in_addr_t addr) {
    auto it = std::find(muted_addr.begin(), muted_addr.end(), addr);
    if (it == muted_addr.end()) {
        return;
    }

    muted_addr.erase(it);

    struct ip_mreq_source source;
    source.imr_interface.s_addr = htonl(INADDR_ANY);
    source.imr_multiaddr.s_addr = htonl(current_group);
    source.imr_sourceaddr.s_addr = addr;

    int result = setsockopt(fd, IPPROTO_IP, IP_UNBLOCK_SOURCE, &source, sizeof(source));
    if (result < 0) {
        perror("setsockopt(IP_UNBLOCK_SOURCE)");
        exit(EXIT_FAILURE);
    }
}

const std::list<in_addr_t>& MultiSocket::getMuted() const {
    return muted_addr;
}

MultiSocketListener::MultiSocketListener(const MultiSocket& multisocket) : multisocket(multisocket) {
    int result = pthread_create(&handle, NULL, worker, this);
    if (result) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }
}

MultiSocketListener::~MultiSocketListener() {
    stop();
}

void MultiSocketListener::stop() {
    pthread_cancel(handle);
}

void* MultiSocketListener::worker(void* obj_ptr) {
    const MultiSocketListener* obj = (const MultiSocketListener*) obj_ptr;

    obj->multisocket.listen();

    pthread_exit(NULL);
}
