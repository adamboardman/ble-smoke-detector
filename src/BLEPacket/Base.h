#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "ble_types.h"
#include "BinaryReader.h"
#include "BinaryWriter.h"
#include "PacketTypes.h"
#include "Peer.h"

class Base {
public:
    Base(uint8_t type);

    Base(uint8_t type, uint8_t ttl, uint64_t timestamp, uint8_t flags);

    Base(uint8_t type, uint8_t ttl, uint64_t timestamp, uint8_t packet_flags, uint64_t sender,
         uint64_t recipient);

    Base(uint8_t type, uint8_t version, BinaryReader &reader);

    virtual ~Base() {}

    void handleReaderRemainder(BinaryReader &reader);

    [[nodiscard]] uint8_t getPacketVersion() const;

    [[nodiscard]] uint8_t getPacketType() const;

    [[nodiscard]] uint8_t getPacketTtl() const;

    [[nodiscard]] uint64_t getPacketTimestamp() const;

    [[nodiscard]] uint8_t getPacketFlags() const;

    void setPacketTtl(uint8_t ttl);

    void setPacketTimestamp(uint64_t timestamp);

    void setPacketFlags(uint8_t flags);

    [[nodiscard]] bool hasPacketRecipient() const {
        return (getPacketFlags() & packet_flag_has_recipient) != 0;
    }

    [[nodiscard]] bool hasSignature() const {
        return (getPacketFlags() & packet_flag_has_signature) != 0;
    }

    [[nodiscard]] bool hasPacketRoute() const {
        return (getPacketFlags() & packet_flag_has_route) != 0;
    }

    [[nodiscard]] bool fromMeshtastic() const {
        return (getPacketFlags() & packet_flag_from_meshtastic) != 0;
    }

    [[nodiscard]] uint64_t getPacketSenderId() const;

    void setPacketSenderId(uint64_t senderId);

    [[nodiscard]] uint64_t getPacketRecipientId() const;

    [[nodiscard]] Peer *getSenderPeer() const;

    void setPayloadLength(uint32_t len);

    [[nodiscard]] virtual uint32_t getPayloadLength();

    void setSenderPeer(Peer *peer);

    [[nodiscard]] virtual std::size_t getPacketHash() const;

    void getPacketSyncId(std::vector<uint8_t> &hash);

    void writePacket(std::vector<uint8_t> &vector);

    void setFromMeshtastic(bool cond);

    void setMalformed(bool cond);

    [[nodiscard]] bool isMalformed() const;

    [[nodiscard]] uint32_t variableLength(uint8_t type, size_t size) const;

    void writeVariable(const BinaryWriter &writer, uint8_t type, const std::string &value);

    void writeVariable(const BinaryWriter &writer, uint8_t type, uint32_t value);

    void writeVariable(const BinaryWriter &writer, uint8_t type, uint8_t value);

    virtual void writePacketPayload(BinaryWriter &writer);

protected:
    void setPacketType(PacketType type);

private:
    bool malformed = false;
    uint8_t packet_version = 1;
    uint8_t packet_type = 0;
    uint8_t packet_ttl = 0;
    uint64_t packet_timestamp_ms = 0;
    uint8_t packet_flags = 0;
    uint32_t payload_length = 0;
    uint64_t packet_sender_id = 0;
    uint64_t packet_recipient_id = 0;
    Peer *sender_peer = nullptr;
    std::vector<uint64_t> routeList{};
    std::string signature{};
};
