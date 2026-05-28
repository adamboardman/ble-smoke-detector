#include "PacketBase.h"

PacketBase::PacketBase(const uint8_t type)
    : Base(type) {
}

PacketBase::PacketBase(const uint8_t type, const uint8_t ttl, const uint64_t timestamp, const uint8_t flags,
                       const uint64_t sender)
    : Base(type, ttl, timestamp, flags),
      packet_sender_id(sender) {
}

PacketBase::PacketBase(const uint8_t type, const uint8_t version, BinaryReader &reader)
    : Base(type, reader) {
    packet_sender_id = reader.read_uint64();
}

uint64_t PacketBase::getPacketSenderId() const {
    return packet_sender_id;
}

void PacketBase::writePacket(std::vector<uint8_t> &vector) {
    Base::writePacket(vector);
}

void PacketBase::writePacketPayload(BinaryWriter &writer) {
    writer.write_uint64(getPacketSenderId());
}
