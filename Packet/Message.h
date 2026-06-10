#pragma once

#include <cstdint>
#include <string>

#include "PacketBase.h"
#include "Peer.h"

class Message final : public PacketBase {
public:
    Message();

    Message(uint8_t ttl, uint64_t timestamp, uint8_t packet_flags, uint64_t sender);

    Message(uint8_t type, uint8_t version, BinaryReader &reader);

    ~Message() override = default;

    void setMessageFlags(uint8_t flags);

    [[nodiscard]] bool isRelay() const {
        return (message_flags & 0x01) != 0;
    }

    [[nodiscard]] bool isPrivate() const {
        return (message_flags & 0x02) != 0;
    }

    [[nodiscard]] bool hasOriginalSender() const {
        return (message_flags & 0x04) != 0;
    }

    [[nodiscard]] bool hasSenderPeerID() const {
        return (message_flags & 0x10) != 0;
    }

    void setMessageTimestamp(uint64_t value);

    void setMessageId(const std::string &string);

    void setSenderNickname(const std::string &string);

    void setContent(const std::string &string);

    void setSenderPeer(Peer *peer);

    [[nodiscard]] uint8_t getMessageFlags() const;

    [[nodiscard]] uint64_t getMessageTimestamp() const;

    [[nodiscard]] const std::string &getMessageId() const;

    [[nodiscard]] const std::string &getSenderNickname() const;

    [[nodiscard]] const std::string &getContent() const;

    [[nodiscard]] Peer *getSenderPeer() const;

    void writePacket(std::vector<uint8_t> &vector) override;

    void writePacketPayload(BinaryWriter &writer) override;

private:
    uint8_t message_flags = 0;
    uint64_t message_timestamp = 0;
    std::string message_id{};
    std::string sender_nickname{};
    std::string content{};
    Peer *sender_peer;
};
