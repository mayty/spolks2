#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#define MESSAGE_SIZE(class) (sizeof(class) - sizeof(IMessage))

#pragma pack(push, 1)

struct IMessage {
    virtual std::vector<unsigned char> build() const = 0;
};

struct Header : public IMessage {
    uint8_t type;
    uint16_t len = MESSAGE_SIZE(Header);
    uint8_t rsv = 0;

    std::vector<unsigned char> build() const;
    static Header parse(const char* message, size_t size);
};

struct DiscoverRequest : public IMessage {
    const static uint8_t MESSAGE_TYPE = 0;

    std::vector<unsigned char> build() const;
};

struct DiscoverResponse : public IMessage {
    const static uint8_t MESSAGE_TYPE = 1;

    std::vector<unsigned char> build() const;
};

struct RoomsRequest : public IMessage {
    const static uint8_t MESSAGE_TYPE = 8;

    std::vector<unsigned char> build() const;
};

struct RoomAnnounce : public IMessage {
    const static uint8_t MESSAGE_TYPE = 9;

    uint32_t address;
    std::string name;

    std::vector<unsigned char> build() const;
    static RoomAnnounce parse(const char* message, size_t size);
};

struct TextMessage : public IMessage {
    const static uint8_t MESSAGE_TYPE = 32;

    std::string text;

    std::vector<unsigned char> build() const;
    static TextMessage parse(const char* message, size_t size);
};

#pragma pack(pop)
