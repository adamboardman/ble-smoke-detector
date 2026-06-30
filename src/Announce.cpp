#include "Announce.h"

#include "int_types.h"

#if defined(PICO_BOARD) || defined(MOCK_PICO_PI)
#include "Debugging.h"
#endif

Announce::Announce()
    : Base(type_announce) {
}

Announce::Announce(const uint8_t type, const uint8_t version, BinaryReader &reader)
    : Base(type, version, reader) {
    uint16_t remainder;
    const auto tail = reader.read_remainder_len();
    do {
        const auto data_type = reader.read_uint8();
        const auto data_length = reader.read_uint8();
        const uint8_t *data = nullptr;
        if (data_type != type_direct_neighbors) {
            data = reader.read_data(data_length);
        }
        if (data) {
            switch (data_type) {
                case type_nickname:
                    name = std::string(reinterpret_cast<const char *>(data), data_length);
                    LOG_DEBUG("data_length: %d, name:%s\n", data_length, name.c_str());
                    break;
                case type_noise_public_key:
                    noise_public_key = std::string(reinterpret_cast<const char *>(data), data_length);
                    LOG_DEBUG("data_length: %d, noise_public_key:%s\n", data_length, noise_public_key.c_str());
                    break;
                case type_signing_public_key:
                    signing_public_key = std::string(reinterpret_cast<const char *>(data), data_length);
                    LOG_DEBUG("data_length: %d, signing_public_key:%s\n", data_length, signing_public_key.c_str());
                    break;
                case type_direct_neighbors:
                    for (int i = 0; i < data_length; i += 8) {
                        auto neighbour = reader.read_uint64();
                        direct_neighbors.push_back(neighbour);
                        LOG_DEBUG("neighbour: %" PRIu64 ", \n", neighbour);
                    }
                    LOG_DEBUG("\n");
                    break;
                default:
                    //ignore unknown future type - data skipped and ignored
                    break;
            }
        } else {
            setMalformed(true);
        }
        remainder = reader.read_remainder_len();
    } while (remainder > tail - getPayloadLength());
    handleReaderRemainder(reader);
}

void Announce::setName(const std::string &value) {
    name = value;
}

const std::string &Announce::getName() const {
    return name;
}

uint32_t Announce::getPayloadLength() {
    if (Base::getPayloadLength() > 0) return Base::getPayloadLength();
    uint32_t len = 0;
    if (name.size() > 0) len += variableLength(type_nickname, name.size());
    if (noise_public_key.size() > 0) len += variableLength(type_noise_public_key, noise_public_key.size());
    if (signing_public_key.size() > 0) len += variableLength(type_signing_public_key, signing_public_key.size());
    if (direct_neighbors.size() > 0) len += variableLength(type_direct_neighbors, direct_neighbors.size());
    setPayloadLength(len);
    return len;
}

void Announce::writePacketPayload(BinaryWriter &writer) {
    if (name.size() > 0) writeVariable(writer, type_nickname, name);
    if (noise_public_key.size() > 0) writeVariable(writer, type_noise_public_key, noise_public_key);
    if (signing_public_key.size() > 0) writeVariable(writer, type_signing_public_key, signing_public_key);
    if (direct_neighbors.size() > 0 && direct_neighbors.size() <= 31) {
        writer.write_uint8(type_direct_neighbors);
        const auto value_len = static_cast<uint8_t>(std::min(static_cast<size_t>(248), direct_neighbors.size() * 8));
        writer.write_uint8(value_len);
        for (const auto value: direct_neighbors) {
            writer.write_uint64(value);
        }
    }
}
