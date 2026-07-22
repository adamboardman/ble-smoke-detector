#include "Message.h"

#include "Debugging.h"

Message::Message() : Base(type_message), sender_peer(nullptr) {
}

Message::Message(const uint8_t ttl, const uint64_t timestamp, const uint8_t packet_flags, const uint64_t sender,
                 const uint64_t recipient)
    : Base(type_message, ttl, timestamp, packet_flags, sender, recipient), sender_peer(nullptr) {
}

Message::Message(const uint8_t version, BinaryReader &reader)
    : Base(type_message, version, reader),
      sender_peer(nullptr) {
    uint16_t remainder;
    const auto tail = reader.read_remainder_len();
    do {
        const auto data_type = reader.read_uint8();
        const auto data_length = reader.read_uint8();
        if (data_type == tlv_message_flags) {
            if (data_length == sizeof(uint8_t)) {
                message_flags = reader.read_uint8();
            } else {
                setMalformed(true);
            }
        } else if (data_type == tlv_message_reply_to_id) {
            if (data_length == sizeof(uint32_t)) {
                reply_id = reader.read_uint32();
            } else {
                setMalformed(true);
            }
        } else if (data_type == tlv_message_location_latitude) {
            if (data_length == sizeof(int32_t)) {
                latitude_i = reader.read_int32();
                LOG_DEBUG("latitude_i: %d\n", latitude_i);
            } else {
                setMalformed(true);
            }
        } else if (data_type == tlv_message_location_longitude) {
            if (data_length == sizeof(int32_t)) {
                longitude_i = reader.read_int32();
                LOG_DEBUG("longitude_i: %d\n", longitude_i);
            } else {
                setMalformed(true);
            }
        } else if (data_type == tlv_message_location_altitude) {
            if (data_length == sizeof(int32_t)) {
                altitude = reader.read_int32();
                LOG_DEBUG("altitude: %d\n", altitude);
            } else {
                setMalformed(true);
            }
        } else if (const uint8_t *data = reader.read_data(data_length)) {
            switch (data_type) {
                case tlv_message_id:
                    message_id = std::string(reinterpret_cast<const char *>(data), data_length);
                    break;
                case tlv_message_content:
                    content = std::string(reinterpret_cast<const char *>(data), data_length);
                    break;
                case tlv_message_sender_nickname:
                    sender_nickname = std::string(reinterpret_cast<const char *>(data), data_length);
                    break;
                case tlv_message_channel:
                    channel = std::string(reinterpret_cast<const char *>(data), data_length);
                    break;
                case tlv_message_channel_content:
                    // We assume that there will never be content and channel content in the same message
                    content = std::string(reinterpret_cast<const char *>(data), data_length);
                    break;
                default:
                    //ignore unknown future type - data skipped and ignored
                    break;
            }
        } else {
            setMalformed(true);
        }
        remainder = reader.read_remainder_len();
    } while (!isMalformed() && remainder > tail - Message::getPayloadLength());
    handleReaderRemainder(reader);
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

void Message::setChannel(const std::string &string) {
    channel = string;
}

void Message::setReplyId(const uint32_t reply_to_id) {
    reply_id = reply_to_id;
}

void Message::setEmoji(const bool cond) {
    if (cond) {
        message_flags |= message_flag_emoji;
    } else {
        message_flags &= ~message_flag_emoji;
    }
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

const std::string &Message::getChannel() const {
    return channel;
}

uint32_t Message::getReplyId() const {
    return reply_id;
}

uint8_t Message::getMessageFlags() const {
    return message_flags;
}

uint32_t Message::getPayloadLength() {
    if (Base::getPayloadLength() > 0) return Base::getPayloadLength();
    uint32_t len = 0;
    if (message_id.size() > 0) len += variableLength(tlv_message_id, message_id.size());
    if (sender_nickname.size() > 0) len += variableLength(tlv_message_sender_nickname, sender_nickname.size());
    if (channel.size() > 0) {
        len += variableLength(tlv_message_channel, channel.size());
        if (content.size() > 0) len += variableLength(tlv_message_channel_content, content.size());
    } else {
        if (content.size() > 0) len += variableLength(tlv_message_content, content.size());
    }
    if (message_flags > 0) len += variableLength(tlv_message_flags, sizeof(uint8_t));
    if (reply_id > 0) len += variableLength(tlv_message_reply_to_id, sizeof(uint32_t));
    if (latitude_i != 0) len += variableLength(tlv_message_location_latitude, sizeof(uint32_t));
    if (longitude_i != 0) len += variableLength(tlv_message_location_longitude, sizeof(uint32_t));
    if (altitude != 0) len += variableLength(tlv_message_location_altitude, sizeof(uint32_t));
    setPayloadLength(len);
    return len;
}

void Message::writePacketPayload(BinaryWriter &writer) {
    if (message_id.size() > 0) {
        writeVariable(writer, tlv_message_id, message_id);
    }
    if (sender_nickname.size() > 0) {
        writeVariable(writer, tlv_message_sender_nickname, sender_nickname);
    }
    if (channel.size() > 0 && content.size() > 0) {
        writeVariable(writer, tlv_message_channel, channel);
        writeVariable(writer, tlv_message_channel_content, content);
    } else if (content.size() > 0) {
        writeVariable(writer, tlv_message_content, content);
    }
    if (message_flags > 0) {
        writeVariable(writer, tlv_message_flags, message_flags);
    }
    if (reply_id > 0) {
        writeVariable(writer, tlv_message_reply_to_id, reply_id);
    }
    if (latitude_i != 0) writeVariable(writer, tlv_message_location_latitude, latitude_i);
    if (longitude_i != 0) writeVariable(writer, tlv_message_location_longitude, longitude_i);
    if (altitude != 0) writeVariable(writer, tlv_message_location_altitude, altitude);
}

void Message::setLatitudeI(const int32_t value) {
    latitude_i = value;
}

void Message::setLongitudeI(const int32_t value) {
    longitude_i = value;
}

void Message::setAltitude(const int32_t value) {
    altitude = value;
}
