#pragma once

#include <cstdint>

#include "include/ble_types.h"
#include "Base.h"
#include "PacketTypes.h"

class PacketBase : public Base {
public:
    PacketBase();

    explicit PacketBase(uint8_t type);

    PacketBase(uint8_t type, uint8_t ttl, uint64_t timestamp, uint8_t flags, uint64_t sender);

    PacketBase(uint8_t type, uint8_t ttl, uint64_t timestamp, uint8_t flags, uint64_t sender, uint64_t recipient);

    PacketBase(uint8_t type, uint8_t version, BinaryReader &reader);

    [[nodiscard]] uint64_t getPacketSenderId() const;

    void setPacketSenderId(uint64_t senderId);

    [[nodiscard]] uint64_t getPacketRecipientId() const;

    [[nodiscard]] bool hasPacketRecipient() const {
        return (getPacketFlags() & packet_flag_has_recipient) != 0;
    }

    [[nodiscard]] virtual std::size_t getPacketHash() const;

    void writePacket(std::vector<uint8_t> &vector) override;

    void writePacketPayload(BinaryWriter &writer) override;

private:
    uint64_t packet_sender_id = 0;
    uint64_t packet_recipient_id = 0;
};
