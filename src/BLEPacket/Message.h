#pragma once

#include <cstdint>
#include <string>

#include "ble_types.h"
#include "Base.h"
#include "Peer.h"

class Message : public Base {
public:
    Message();

    Message(uint8_t ttl, uint64_t timestamp, uint8_t packet_flags, uint64_t sender, uint64_t recipient);

    Message(uint8_t version, BinaryReader &reader);

    ~Message() override {};

    void setMessageId(const std::string &string);

    void setSenderNickname(const std::string &string);

    void setContent(const std::string &string);

    void setChannel(const std::string &string);

    void setReplyId(uint32_t reply_to_id);

    void setEmoji(bool cond);

    [[nodiscard]] const std::string &getMessageId() const;

    [[nodiscard]] const std::string &getSenderNickname() const;

    [[nodiscard]] const std::string &getContent() const;

    [[nodiscard]] const std::string &getChannel() const;

    [[nodiscard]] uint32_t getReplyId() const;

    [[nodiscard]] uint8_t getMessageFlags() const;

    [[nodiscard]] uint32_t getPayloadLength() override;

    void writePacketPayload(BinaryWriter &writer) override;

private:
    std::string message_id{};
    std::string sender_nickname{};
    std::string content{};
    uint32_t reply_id = 0;
    uint8_t message_flags = 0;
    Peer *sender_peer;
    std::string channel{};
};
