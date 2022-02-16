#include "protocol.h"

#include <arpa/inet.h>

template<class T>
void write_uchar_vector(std::vector<unsigned char>& vector, T value) {
    unsigned char* ptr = (unsigned char*) &value;
    vector.insert(vector.end(), ptr, ptr + sizeof(T));
}

template <>
void write_uchar_vector<std::string>(std::vector<unsigned char>& vector, std::string value) {
    vector.insert(vector.end(), value.begin(), value.end());
}

std::vector<unsigned char> build_header(uint8_t type) {
    Header header;
    header.type = type;

    return header.build();
}

std::vector<unsigned char> Header::build() const {
    std::vector<unsigned char> header;

    write_uchar_vector(header, type);
    write_uchar_vector(header, htons(len));
    write_uchar_vector(header, rsv);

    return header;
}

Header Header::parse(const char* message, size_t size) {
    Header result;

    result.type = message[0];
    result.len = ntohs( *((uint16_t*) &message[1]) );
    result.rsv = message[3];

    return result;
}

#define DEFINE_EMPTY_MSG_BUILDER(Type) \
    std::vector<unsigned char> Type::build() const { \
        return build_header(MESSAGE_TYPE);\
    }

DEFINE_EMPTY_MSG_BUILDER(DiscoverRequest)
DEFINE_EMPTY_MSG_BUILDER(DiscoverResponse)
DEFINE_EMPTY_MSG_BUILDER(RoomsRequest)

std::vector<unsigned char> RoomAnnounce::build() const {
    Header header;
    header.type = MESSAGE_TYPE;
    header.len += sizeof(address) + name.size();

    auto msg = header.build();

    write_uchar_vector(msg, htonl(address));
    write_uchar_vector(msg, name);

    return msg;
}

RoomAnnounce RoomAnnounce::parse(const char* message, size_t size) {
    RoomAnnounce announce;

    announce.address = ntohl( *((uint32_t*) message) );
    announce.name = std::string(&message[4], size - 4);

    return announce;
}

std::vector<unsigned char> TextMessage::build() const {
    Header header;
    header.type = MESSAGE_TYPE;
    header.len += text.size();

    auto msg = header.build();

    write_uchar_vector(msg, text);

    return msg;
}

TextMessage TextMessage::parse(const char* message, size_t size) {
    TextMessage txtmsg;

    txtmsg.text = std::string(message, size);

    return txtmsg;
}
