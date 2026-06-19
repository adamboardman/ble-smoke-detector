#include "PacketBase.h"

#include "include/int_types.h"

#if defined(PICO_BOARD) || defined(MOCK_PICO_PI)
#include "Debugging.h"
#endif

PacketBase::PacketBase()
    : Base(type_unknown) {
}

PacketBase::PacketBase(const uint8_t type)
    : Base(type) {
}

PacketBase::PacketBase(const uint8_t type, const uint8_t ttl, const uint64_t timestamp, const uint8_t flags,
                       const uint64_t sender)
    : Base(type, ttl, timestamp, flags),
      packet_sender_id(sender) {
}

PacketBase::PacketBase(const uint8_t type, const uint8_t ttl, const uint64_t timestamp, const uint8_t flags,
                       const uint64_t sender, const uint64_t recipient)
    : Base(type, ttl, timestamp, flags),
      packet_sender_id(sender),
      packet_recipient_id(recipient) {
}

PacketBase::PacketBase(const uint8_t type, const uint8_t version, BinaryReader &reader)
    : Base(type, reader) {
    packet_sender_id = reader.read_uint64();
    LOG_DEBUG("sender_id: 0x%" PRIx64 "\n", packet_sender_id);
    if (hasPacketRecipient()) {
        packet_recipient_id = reader.read_uint64();
        LOG_DEBUG("recipient_id: 0x%" PRIx64 "\n", packet_recipient_id);
    }
}

uint64_t PacketBase::getPacketSenderId() const {
    return packet_sender_id;
}

void PacketBase::setPacketSenderId(const uint64_t senderId) {
    packet_sender_id = senderId;
}

uint64_t PacketBase::getPacketRecipientId() const {
    return packet_recipient_id;
}

std::size_t PacketBase::getPacketHash() const {
    std::vector<uint8_t> meta_buffer;
    const BinaryWriter writer(meta_buffer);
    writer.write_uint8(getPacketType());
    //ignore ttl for hash as we want to ignore the same message going around again
    writer.write_uint8(getPacketFlags());
    writer.write_uint64(getPacketTimestamp());
    writer.write_uint64(getPacketSenderId());
    writer.write_uint64(getPacketRecipientId());
    std::string meta_string;
    meta_string.assign(reinterpret_cast<const char *>(meta_buffer.data()), meta_buffer.size());
    return std::hash<std::string>{}(meta_string);
}

void PacketBase::writePacket(std::vector<uint8_t> &vector) {
    Base::writePacket(vector);
}

void PacketBase::writePacketPayload(BinaryWriter &writer) {
    writer.write_uint64(getPacketSenderId());
    if (hasPacketRecipient()) {
        writer.write_uint64(getPacketRecipientId());
    }
}
