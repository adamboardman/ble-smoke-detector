#include "PacketBase.h"

PacketBase::PacketBase(const uint8_t type)
    : packet_type(type) {
}

PacketBase::PacketBase(const uint8_t type, const uint8_t ttl, const uint64_t timestamp, const uint8_t flags,
                       const uint64_t sender)
    : packet_type(type),
      packet_ttl(ttl),
      packet_timestamp(timestamp),
      packet_flags(flags),
      packet_sender_id(sender) {
}


uint8_t PacketBase::getPacketType() const {
    return packet_type;
}

uint8_t PacketBase::getPacketTtl() const {
    return packet_ttl;
}

uint64_t PacketBase::getPacketTimestamp() const {
    return packet_timestamp;
}

uint64_t PacketBase::getPacketTimestampMs() const {
    return packet_timestamp / 1000;
}

uint8_t PacketBase::getPacketFlags() const {
    return packet_flags;
}

uint64_t PacketBase::getPacketSenderId() const {
    return packet_sender_id;
}

void PacketBase::setPacketTtl(const uint8_t ttl) {
    packet_ttl = ttl;
}

void PacketBase::setPacketTimestamp(const uint64_t timestamp) {
    packet_timestamp = timestamp;
}

void PacketBase::setPacketFlags(const uint8_t flags) {
    packet_flags = flags;
}

void PacketBase::setPacketSenderId(const uint64_t senderId) {
    packet_sender_id = senderId;
}

void PacketBase::setPacketType(PacketType type) {
    packet_type = type;
}
