#pragma once

#include <chrono>
#include <functional>
#include <list>
#include <map>
#include <string>
#include <netinet/in.h>

#include "network.h"
#include "protocol.h"

struct Room {
    std::string name;
    in_addr_t addr;

    bool operator==(const Room& other) const {
        return addr == other.addr;
    }

    bool operator!=(const Room& other) const {
        return !(*this == other);
    }

    Room& operator=(const Room& other) {
        name = other.name;
        addr = other.addr;

        return *this;
    }

    bool operator<(const Room& other) const {
        return addr < other.addr;
    }
};

const Room BROADCAST_ROOM = {
    "All",
    INADDR_BROADCAST,
};

class Controller {
public:
    const static unsigned short DEFAULT_PORT = 12000;

    using ReceiveCallback = std::function<void (in_addr_t sender, std::string text)>;

    Controller();

    void send(std::string text);
    void setOnReceive(ReceiveCallback callback);

    const std::list<Room> getRooms();
    const Room getCurrentRoom() const;
    const Room createRoom(std::string name);
    bool join(const Room& room);
    void leave();

    const std::list<in_addr_t> getClients();
    const std::list<in_addr_t>& getMuted() const;
    void mute(in_addr_t addr);
    void unmute(in_addr_t addr);

    void searchClients();
    void announceClient();
    void searchRooms();
    void announceCurrentRoom();

protected:
    MultiSocket multisocket;
    MultiSocketListener listener;

    ReceiveCallback recv_callback = nullptr;

    std::map<in_addr_t, std::chrono::time_point<std::chrono::steady_clock>> clients;

    std::map<Room, std::chrono::time_point<std::chrono::steady_clock>> rooms;
    Room current_room = BROADCAST_ROOM;

    void sendMessage(const IMessage& message, bool force_broadcast = false);
    void onReceive(in_addr_t sender, const char* data, size_t size);

    void announceRoom(const Room& room);
};
