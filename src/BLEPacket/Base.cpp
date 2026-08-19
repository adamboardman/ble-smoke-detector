#include "Base.h"

#include <cinttypes>
#include <string>
#include <utility>
#include <vector>
#include "assert.h"
#include "BinaryWriter.h"
#include "int_types.h"
#include "Debugging.h"

constexpr uint16_t SIGNATURE_LENGTH = 64;

Base::Base(const uint8_t type)
    : packet_type(type) {
}

Base::Base(const uint8_t type, const uint8_t ttl, const uint64_t timestamp, const uint8_t flags)
    : packet_type(type),
      packet_ttl(ttl),
      packet_timestamp_ms(timestamp),
      packet_flags(flags) {
}

Base::Base(const uint8_t type, const uint8_t ttl, const uint64_t timestamp, const uint8_t flags,
           const uint64_t sender, const uint64_t recipient)
    : packet_type(type),
      packet_ttl(ttl),
      packet_timestamp_ms(timestamp),
      packet_flags(flags),
      packet_sender_id(sender),
      packet_recipient_id(recipient) {
}

Base::Base(const uint8_t type, const uint8_t version, BinaryReader &reader)
    : packet_type(type) {
    packet_ttl = reader.read_uint8();
    LOG_DEBUG("ttl: %d\n", packet_ttl);
    packet_timestamp_ms = reader.read_uint64();
    LOG_DEBUG("timestamp: 0x%" PRIx64 "\n", packet_timestamp_ms);
    packet_flags = reader.read_uint8();
    LOG_DEBUG("flags: %d\n", packet_flags);

    if (version == 1) {
        payload_length = reader.read_uint16();
        LOG_DEBUG("payload length: %d\n", payload_length);
    } else if (version == 2) {
        payload_length = reader.read_uint32();
        LOG_DEBUG("payload length: %d\n", payload_length);
    }
    packet_sender_id = reader.read_uint64();
    LOG_DEBUG("sender: 0x%" PRIx64 "\n", packet_sender_id);
    if (hasPacketRecipient()) {
        packet_recipient_id = reader.read_uint64();
        LOG_DEBUG("recipient: 0x%" PRIx64 "\n", packet_recipient_id);
    }
    if (version == 2 && hasPacketRoute()) {
        const auto routeCount = reader.read_uint8();
        routeList.clear();
        for (auto i = 0; i < routeCount; i++) {
            auto hop_id = reader.read_uint64();
            routeList.push_back(hop_id);
        }
    }
}

void Base::handleReaderRemainder(BinaryReader &reader) {
    if (!isMalformed() && hasSignature()) {
        if (const auto sig_data = reader.read_data(SIGNATURE_LENGTH)) {
            signature = std::string(reinterpret_cast<const char *>(sig_data), SIGNATURE_LENGTH);
        } else {
            signature = "";
            malformed = true;
        }
    }
    //We ignore pointless padding
}

uint8_t Base::getPacketVersion() const {
    return packet_version;
}

uint8_t Base::getPacketType() const {
    return packet_type;
}

uint8_t Base::getPacketTtl() const {
    return packet_ttl;
}

uint64_t Base::getPacketTimestamp() const {
    return packet_timestamp_ms;
}

uint8_t Base::getPacketFlags() const {
    return packet_flags;
}

void Base::setPacketTtl(const uint8_t ttl) {
    packet_ttl = ttl;
}

void Base::setPacketTimestamp(const uint64_t timestamp) {
    packet_timestamp_ms = timestamp;
}

void Base::setPacketFlags(const uint8_t flags) {
    packet_flags = flags;
}

void Base::setSendingConnectionHandle(const uint16_t handle) {
    sending_connection_handle = handle;
}

uint16_t Base::getSendingConnectionHandle() {
    return sending_connection_handle;
}

void Base::setPacketType(const PacketType type) {
    packet_type = type;
}

uint64_t Base::getPacketSenderId() const {
    return packet_sender_id;
}

void Base::setPacketSenderId(const uint64_t senderId) {
    packet_sender_id = senderId;
}

uint64_t Base::getPacketRecipientId() const {
    return packet_recipient_id;
}

void Base::setSenderPeer(Peer *peer) {
    sender_peer = peer;
}

Peer *Base::getSenderPeer() const {
    return sender_peer;
}

void Base::setPayloadLength(const uint32_t len) {
    payload_length = len;
}

uint32_t Base::getPayloadLength() {
    return payload_length;
}

std::size_t Base::getPacketHash() const {
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

void Base::writePacket(std::vector<uint8_t> &vector) {
    BinaryWriter writer(vector);

    writer.write_uint8(getPacketVersion());
    writer.write_uint8(getPacketType());

    writer.write_uint8(getPacketTtl());

    writer.write_uint64(getPacketTimestamp());
    writer.write_uint8(getPacketFlags());

    if (getPacketVersion() == 1) {
        writer.write_uint16(getPayloadLength());
    } else if (getPacketVersion() == 2) {
        writer.write_uint32(getPayloadLength());
    }
    writer.write_uint64(getPacketSenderId());
    if (hasPacketRecipient()) {
        writer.write_uint64(getPacketRecipientId());
    }
    if (getPacketVersion() == 2 && hasPacketRoute()) {
        writer.write_uint8(routeList.size());
        for (const auto i: routeList) {
            writer.write_uint64(i);
        }
    }

    writePacketPayload(writer);

    if (hasSignature()) {
        writer.write_uint8(signature.size());
        writer.write_data(signature, signature.size());
    }
    //We add zero length padding for compatibility - it is pointless outside an encrypted block
    writer.write_uint8(0);
}

void Base::setFromMeshtastic(const bool cond) {
    if (cond) {
        packet_flags |= packet_flag_from_meshtastic;
    } else {
        packet_flags &= ~packet_flag_from_meshtastic;
    }
}

void Base::setMalformed(const bool cond) {
    malformed = cond;
}

bool Base::isMalformed() const {
    return malformed;
}

uint32_t Base::variableLength(const uint8_t type, const size_t size) const {
    uint32_t value_len = 2;
    if (packet_type == type_announce && type == tlv_announce_direct_neighbors) {
        value_len += static_cast<uint8_t>(std::min(static_cast<size_t>(248), size * 8));
    } else {
        value_len += static_cast<uint8_t>(std::min(static_cast<size_t>(255), size));
    }
    return value_len;
}

void Base::writeVariable(const BinaryWriter &writer, uint8_t type, const std::string &value) {
    writer.write_uint8(type);
    const auto value_len = static_cast<uint8_t>(std::min(static_cast<size_t>(255), value.size()));
    writer.write_uint8(value_len);
    writer.write_data(value, value_len);
}

void Base::writeVariable(const BinaryWriter &writer, uint8_t type, const uint32_t value) {
    writer.write_uint8(type);
    writer.write_uint8(sizeof(value));
    writer.write_uint32(value);
}

void Base::writeVariable(const BinaryWriter &writer, uint8_t type, const int32_t value) {
    writer.write_uint8(type);
    writer.write_uint8(sizeof(value));
    writer.write_int32(value);
}

void Base::writeVariable(const BinaryWriter &writer, uint8_t type, const uint8_t value) {
    writer.write_uint8(type);
    writer.write_uint8(sizeof(value));
    writer.write_uint8(value);
}

void Base::writePacketPayload(BinaryWriter &writer) {
    ASSERT_DEBUG(false);
}
