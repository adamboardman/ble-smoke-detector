#include "Announce.h"
#include "int_types.h"
#include "Debugging.h"

Announce::Announce()
    : Base(type_announce) {
}

Announce::Announce(const uint8_t version, BinaryReader &reader)
    : Base(type_announce, version, reader) {
    uint16_t remainder;
    const auto tail = reader.read_remainder_len();
    do {
        const auto data_type = reader.read_uint8();
        const auto data_length = reader.read_uint8();
        if (data_type == tlv_announce_location_latitude) {
            if (data_length == sizeof(int32_t)) {
                latitude_i = reader.read_int32();
            } else {
                setMalformed(true);
            }
        } else if (data_type == tlv_announce_location_longitude) {
            if (data_length == sizeof(int32_t)) {
                longitude_i = reader.read_int32();
            } else {
                setMalformed(true);
            }
        } else if (data_type == tlv_announce_location_altitude) {
            if (data_length == sizeof(int32_t)) {
                altitude = reader.read_int32();
            } else {
                setMalformed(true);
            }
        } else if (data_type == tlv_announce_direct_neighbors) {
            for (int i = 0; i < data_length; i += 8) {
                auto neighbour = reader.read_uint64();
                direct_neighbors.push_back(neighbour);
                LOG_DEBUG("neighbour: %" PRIx64 ", ", neighbour);
            }
            LOG_DEBUG("\n");
        } else if (const uint8_t *data = reader.read_data(data_length)) {
            switch (data_type) {
                case tlv_announce_nickname:
                    name = std::string(reinterpret_cast<const char *>(data), data_length);
                    LOG_DEBUG("data_length: %d, name:%s\n", data_length, name.c_str());
                    break;
                case tlv_announce_noise_public_key:
                    noise_public_key = std::string(reinterpret_cast<const char *>(data), data_length);
#if defined(PICO_BOARD) || defined(MOCK_PICO_PI)
                    print_named_data("noise_public_key", data, data_length);
#endif
                    break;
                case tlv_announce_signing_public_key:
                    signing_public_key = std::string(reinterpret_cast<const char *>(data), data_length);
#if defined(PICO_BOARD) || defined(MOCK_PICO_PI)
                    print_named_data("signing_public_key", data, data_length);
#endif
                    break;
                default:
                    //ignore unknown future type - data skipped and ignored
                    break;
            }
        } else {
            setMalformed(true);
        }
        remainder = reader.read_remainder_len();
    } while (!isMalformed() && remainder > tail - Announce::getPayloadLength());
    handleReaderRemainder(reader);
}

void Announce::setName(const std::string &value) {
    name = value;
}

void Announce::setNoisePublicKey(const std::string &value) {
    noise_public_key = value;
}

void Announce::setSigningPublicKey(const std::string &value) {
    signing_public_key = value;
}

const std::string &Announce::getName() const {
    return name;
}

const std::string &Announce::getSigningPublicKey() const {
    return signing_public_key;
}

uint32_t Announce::getPayloadLength() {
    if (Base::getPayloadLength() > 0) return Base::getPayloadLength();
    uint32_t len = 0;
    if (name.size() > 0) len += variableLength(tlv_announce_nickname, name.size());
    if (noise_public_key.size() > 0) len += variableLength(tlv_announce_noise_public_key, noise_public_key.size());
    if (signing_public_key.size() > 0)
        len += variableLength(tlv_announce_signing_public_key, signing_public_key.size());
    if (direct_neighbors.size() > 0) len += variableLength(tlv_announce_direct_neighbors, direct_neighbors.size());
    if (latitude_i != 0) len += variableLength(tlv_announce_location_latitude, sizeof(uint32_t));
    if (longitude_i != 0) len += variableLength(tlv_announce_location_longitude, sizeof(uint32_t));
    if (altitude != 0) len += variableLength(tlv_announce_location_altitude, sizeof(uint32_t));
    setPayloadLength(len);
    return len;
}

/**
 * Get a hash of the important parts of the announcement.
 * We ignore time and ttl as they are not important to compare
 * - we only want to store the most recent announce for each
 *
 * @return hash - a small representation of the uniqueness of the sender of this announcement
 */
std::size_t Announce::getAnnounceHash() const {
    std::vector<uint8_t> meta_buffer;
    const BinaryWriter writer(meta_buffer);
    writer.write_uint8(getPacketType());
    writer.write_uint8(getPacketFlags());
    writer.write_uint64(getPacketSenderId());
    writer.write_uint64(getPacketRecipientId());
    writer.write_int32(latitude_i);
    writer.write_int32(longitude_i);
    writer.write_int32(altitude);
    std::string meta_string;
    meta_string.assign(reinterpret_cast<const char *>(meta_buffer.data()), meta_buffer.size());
    return std::hash<std::string>{}(meta_string);
}

void Announce::writePacketPayload(BinaryWriter &writer) {
    if (name.size() > 0) writeVariable(writer, tlv_announce_nickname, name);
    if (noise_public_key.size() > 0) writeVariable(writer, tlv_announce_noise_public_key, noise_public_key);
    if (signing_public_key.size() > 0) writeVariable(writer, tlv_announce_signing_public_key, signing_public_key);
    if (direct_neighbors.size() > 0 && direct_neighbors.size() <= 31) {
        writer.write_uint8(tlv_announce_direct_neighbors);
        const auto value_len = static_cast<uint8_t>(std::min(static_cast<size_t>(248), direct_neighbors.size() * 8));
        writer.write_uint8(value_len);
        for (const auto value: direct_neighbors) {
            writer.write_uint64(value);
        }
    }
    if (latitude_i != 0) writeVariable(writer, tlv_message_location_latitude, latitude_i);
    if (longitude_i != 0) writeVariable(writer, tlv_message_location_longitude, longitude_i);
    if (altitude != 0) writeVariable(writer, tlv_message_location_altitude, altitude);
}

void Announce::setLatitudeI(const int32_t value) {
    latitude_i = value;
}

void Announce::setLongitudeI(const int32_t value) {
    longitude_i = value;
}

void Announce::setAltitude(const int32_t value) {
    altitude = value;
}
