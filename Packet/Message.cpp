#include "Message.h"

#include "PacketTypes.h"

Message::Message() : PacketBase(type_message), sender_peer(nullptr) {
}

Message::Message(const uint8_t ttl, const uint64_t timestamp, const uint8_t packet_flags, const uint64_t sender)
    : PacketBase(type_message, ttl, timestamp, packet_flags, sender), sender_peer(nullptr) {
}

void Message::setMessageFlags(const uint8_t flags) {
    message_flags = flags;
}

void Message::setMessageTimestamp(uint64_t value) {
    message_timestamp = value;
}

void Message::setMessageId(const std::string &string) {
    message_id = string;
}

void Message::setSenderNickname(const std::string &string) {
    sender_nickname = string;
}

void Message::setContent(const std::string &string) {
    content = string;
}

uint8_t Message::getMessageFlags() const {
    return message_flags;
}

uint64_t Message::getMessageTimestamp() const {
    return message_timestamp;
}

const std::string &Message::getMessageId() const {
    return message_id;
}

const std::string &Message::getSenderNickname() const {
    return sender_nickname;
}

const std::string &Message::getContent() const {
    return content;
}

void Message::setSenderPeer(Peer *peer) {
    sender_peer = peer;
}

Peer *Message::getSenderPeer() const {
    return sender_peer;
}

