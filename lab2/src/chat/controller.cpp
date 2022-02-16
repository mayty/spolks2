#include "controller.h"

#include <algorithm>

Controller::Controller() : multisocket(DEFAULT_PORT), listener(multisocket) {
    auto bindedCallback = std::bind(
        &Controller::onReceive,
        this,
        std::placeholders::_1,
        std::placeholders::_2,
        std::placeholders::_3
    );
    multisocket.setOnReceive(bindedCallback);

    announceClient();

    searchClients();
    searchRooms();
}

void Controller::send(std::string text) {
    TextMessage message;
    message.text = text;

    sendMessage(message);
}

void Controller::setOnReceive(ReceiveCallback callback) {
    recv_callback = callback;
}

void Controller::sendMessage(const IMessage& message, bool force_broadcast) {
    auto data = message.build();

    multisocket.send(data.data(), data.size(), force_broadcast);
}

const std::list<Room> Controller::getRooms() {
    std::list<Room> result;

    auto deadline = std::chrono::steady_clock::now() - std::chrono::minutes(1);

    auto it = rooms.begin();
    for (; it != rooms.end();) {
        if (it->second > deadline or it->first == current_room) {
            it++;
            continue;
        }

        it = rooms.erase(it);
    }

    for (auto& pair : rooms) {
        result.push_back(pair.first);
    }

    return result;
}

const Room Controller::getCurrentRoom() const {
    return current_room;
}

const Room Controller::createRoom(std::string name) {
    Room room = { name, 0xE0000000}; // 224.0.0.0
    const in_addr_t limit = 0xE07FFFFF; // 224.127.255.255

    for (; room.addr <= limit; room.addr++) {
        bool exist = rooms.count(room);

        if (!exist) {
            break;
        }
    }

    rooms[room] = std::chrono::steady_clock::now();

    announceRoom(room);

    return room;
}

bool Controller::join(const Room& room) {
    leave();

    if (room == BROADCAST_ROOM) {
        current_room = BROADCAST_ROOM;
        return true;
    }

    bool exist = rooms.count(room);
    if (!exist) {
        return false;
    }

    multisocket.join(room.addr);

    current_room = room;

    return true;
}

void Controller::leave() {
    if (current_room == BROADCAST_ROOM) {
        return;
    }

    multisocket.leave();

    current_room = BROADCAST_ROOM;
}

const std::list<in_addr_t> Controller::getClients() {
    std::list<in_addr_t> result;

    auto deadline = std::chrono::steady_clock::now() - std::chrono::minutes(1);

    auto it = clients.begin();
    for (; it != clients.end();) {
        if (it->second > deadline) {
            it++;
            continue;
        }

        it = clients.erase(it);
    }

    for (auto& pair : clients) {
        result.push_back(pair.first);
    }

    return result;
}

const std::list<in_addr_t>& Controller::getMuted() const {
    return multisocket.getMuted();
}

void Controller::mute(in_addr_t addr) {
    multisocket.mute(addr);
}

void Controller::unmute(in_addr_t addr) {
    multisocket.unmute(addr);
}

void Controller::onReceive(in_addr_t sender, const char* data, size_t size) {
    if (size < MESSAGE_SIZE(Header)) {
        return;
    }

    Header header = Header::parse(data, size);

    if (header.len != size) {
        return;
    }

    const char* payload = data + MESSAGE_SIZE(Header);
    size_t payload_size = size - MESSAGE_SIZE(Header);

    switch (header.type) {
        case DiscoverRequest::MESSAGE_TYPE:
            announceClient();
            break;

        case DiscoverResponse::MESSAGE_TYPE:
            clients[sender] = std::chrono::steady_clock::now();
            break;

        case RoomsRequest::MESSAGE_TYPE:
            announceCurrentRoom();
            break;

        case RoomAnnounce::MESSAGE_TYPE: {
            RoomAnnounce announce = RoomAnnounce::parse(payload, payload_size);

            Room room = { announce.name, announce.address };
            rooms[room] = std::chrono::steady_clock::now();

            break;
        }

        case TextMessage::MESSAGE_TYPE: {
            TextMessage message = TextMessage::parse(payload, payload_size);

            if (recv_callback) {
                recv_callback(sender, message.text);
            }

            break;
        }
    }
}

void Controller::searchClients() {
    DiscoverRequest request;

    sendMessage(request, true);
}

void Controller::announceClient() {
    DiscoverResponse response;

    sendMessage(response, true);
}

void Controller::searchRooms() {
    RoomsRequest request;

    sendMessage(request, true);
}

void Controller::announceRoom(const Room& room) {
    RoomAnnounce announce;

    announce.name = room.name;
    announce.address = room.addr;

    sendMessage(announce, true);
}

void Controller::announceCurrentRoom() {
    if (current_room == BROADCAST_ROOM) {
        return;
    }

    announceRoom(current_room);
}
