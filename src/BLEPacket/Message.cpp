#include "Message.h"

#if defined(PICO_BOARD) || defined(MOCK_PICO_PI)
extern void print_named_data(const char *name, const uint8_t *data, uint16_t data_size);
#endif

Message::Message() : Base(type_message) {
}

Message::Message(const uint8_t ttl, const uint64_t timestamp, const uint8_t packet_flags, const uint64_t sender,
                 const uint64_t recipient)
    : Base(type_message, ttl, timestamp, packet_flags, sender, recipient) {
}

Message::Message(const uint8_t type, const uint8_t version, BinaryReader &reader)
    : Base(type, version, reader) {
    message_flags = reader.read_uint8();
    message_timestamp_ms = reader.read_uint64();

    const auto message_id_len = reader.read_uint8();
    if (const auto message_id_data = reader.read_data(message_id_len)) {
#if defined(PICO_BOARD) || defined(MOCK_PICO_PI)
        print_named_data("message_id", message_id_data, message_id_len);
#endif
        message_id = std::string(reinterpret_cast<const char *>(message_id_data), message_id_len);
    } else {
        message_id = "";
    }

    const auto sender_nickname_len = reader.read_uint8();
    if (const auto sender_nick_data = reader.read_data(sender_nickname_len)) {
        sender_nickname = std::string(reinterpret_cast<const char *>(sender_nick_data), sender_nickname_len);
#if defined(PICO_BOARD) || defined(MOCK_PICO_PI)
        print_named_data("sender_nick", sender_nick_data, sender_nickname_len);
#else
        LOG_DEBUG("Nick: %s\n", sender_nickname.c_str());
#endif
    } else {
        sender_nickname = "";
    }

    const auto content_len = reader.read_uint16();
    if (const auto content_data = reader.read_data(content_len)) {
        content = std::string(reinterpret_cast<const char *>(content_data), content_len);
#if defined(PICO_BOARD) || defined(MOCK_PICO_PI)
        print_named_data("content", content_data, content_len);
#else
        LOG_DEBUG("Content: %s\n", content.c_str());
#endif
    } else {
        content = "";
    }

    if (hasSenderPeerID()) {
        const auto id_len = reader.read_uint8();
        if (id_len == sizeof(uint64_t) * 2) {
            sender_peer_id = reader.read_uint64();
        } else if (const auto id_data = reader.read_data(id_len)) {
#if defined(PICO_BOARD) || defined(MOCK_PICO_PI)
            print_named_data("id_data", id_data, id_len);
#endif
            //ignore unusable data
        }
    }

    if (hasChannel()) {
        const auto channel_len = reader.read_uint8();
        if (const auto channel_data = reader.read_data(channel_len)) {
#if defined(PICO_BOARD) || defined(MOCK_PICO_PI)
            print_named_data("channel", channel_data, channel_len);
#endif
            channel = std::string(reinterpret_cast<const char *>(channel_data), channel_len);
        } else {
            channel = "";
        }
    }

    handleReaderRemainder(reader);
}

void Message::setMessageFlags(const uint8_t flags) {
    message_flags = flags;
}

void Message::setMessageTimestamp(const uint64_t value) {
    message_timestamp_ms = value;
}

void Message::setMessageId(const std::string &string) {
    message_id = string;
}

void Message::setSenderNickname(const std::string &string) {
    sender_nickname = string;
}

void Message::setContent(const std::string &string) {
    content = string;
    setPayloadLength(0); //ensure re-calculation
}

void Message::setEncryptedContent(const std::string &string) {
    encrypted_content = string;
}

void Message::setRecipientNickname(const std::string &string) {
    recipient_nickname = string;
}

void Message::setChannel(const std::string &string) {
    if (!string.empty()) {
        message_flags |= message_flag_has_channel;
    }
    channel = string;
}

uint8_t Message::getMessageFlags() const {
    return message_flags;
}

uint64_t Message::getMessageTimestamp() const {
    return message_timestamp_ms;
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

const std::string &Message::getRecipientNickname() const {
    return recipient_nickname;
}

const std::string &Message::getChannel() const {
    return channel;
}

uint32_t Message::getPayloadLength() {
    if (Base::getPayloadLength() > 0) return Base::getPayloadLength();
    uint32_t len = 0;
    len += 1; //message_flags
    len += sizeof(uint64_t); //message_timestamp_ms
    len += 1 + static_cast<uint8_t>(std::min(static_cast<size_t>(255), message_id.size()));
    len += 1 + static_cast<uint8_t>(std::min(static_cast<size_t>(255), sender_nickname.size()));
    len += 1 + static_cast<uint16_t>(std::min(static_cast<size_t>(65535), content.size()));
    if (hasSenderPeerID()) {
        len += 1; //size
        len += sizeof(uint64_t); //peerId
    }
    if (hasChannel()) {
        len += 1 + static_cast<uint8_t>(std::min(static_cast<size_t>(255), channel.size()));
    }
    setPayloadLength(len);
    return len;
}

void Message::writePacketPayload(BinaryWriter &writer) {
    writer.write_uint8(message_flags);
    writer.write_uint64(message_timestamp_ms);

    const auto id_len = static_cast<uint8_t>(std::min(static_cast<size_t>(255), message_id.size()));
    writer.write_uint8(id_len);
    writer.write_data(message_id, id_len);

    const auto sender_len = static_cast<uint8_t>(std::min(static_cast<size_t>(255), sender_nickname.size()));
    writer.write_uint8(sender_len);
    writer.write_data(sender_nickname, sender_len);

    const auto content_len = static_cast<uint16_t>(std::min(static_cast<size_t>(65535), content.size()));
    writer.write_uint16(content_len);
    writer.write_data(content, content_len);

    if (hasSenderPeerID()) {
        const auto peerId = getSenderPeer()->getId();
        constexpr uint8_t size = sizeof(uint64_t) * 2;
        writer.write_uint8(size);
        writer.write_uint64(peerId);
    }

    if (hasChannel()) {
        const auto channel_len = static_cast<uint8_t>(std::min(static_cast<size_t>(255), channel.size()));
        writer.write_uint8(channel_len);
        writer.write_data(channel, channel_len);
    }
}
