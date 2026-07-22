#include "BinaryReader.h"

#include <algorithm>

uint8_t BinaryReader::read_uint8() {
    if (pos + 1 > buffer_size) {
        pos += 1;
        return 0;
    }
    uint8_t ret = buffer[pos];
    pos += 1;
    return ret;
}

uint16_t BinaryReader::read_uint16() {
    if (pos + 2 > buffer_size) {
        pos += 2;
        return 0;
    }
    const uint16_t ret = static_cast<uint16_t>(buffer[pos]) << 8 | buffer[pos + 1];
    pos += 2;
    return ret;
}

int32_t BinaryReader::read_int32() {
    if (pos + 4 > buffer_size) {
        pos += 4;
        return 0;
    }
    const uint32_t ret = static_cast<uint64_t>(buffer[pos]) << 24 |
                         static_cast<uint64_t>(buffer[pos + 1]) << 16 |
                         static_cast<uint64_t>(buffer[pos + 2]) << 8 |
                         static_cast<uint64_t>(buffer[pos + 3]);
    pos += 4;
    return ret;
}

uint32_t BinaryReader::read_uint32() {
    if (pos + 4 > buffer_size) {
        pos += 4;
        return 0;
    }
    const uint32_t ret = static_cast<uint64_t>(buffer[pos]) << 24 |
                         static_cast<uint64_t>(buffer[pos + 1]) << 16 |
                         static_cast<uint64_t>(buffer[pos + 2]) << 8 |
                         static_cast<uint64_t>(buffer[pos + 3]);
    pos += 4;
    return ret;
}

uint64_t BinaryReader::read_uint64() {
    if (pos + 8 > buffer_size) {
        pos += 8;
        return 0;
    }
    const uint64_t ret = static_cast<uint64_t>(buffer[pos]) << 56 |
                         static_cast<uint64_t>(buffer[pos + 1]) << 48 |
                         static_cast<uint64_t>(buffer[pos + 2]) << 40 |
                         static_cast<uint64_t>(buffer[pos + 3]) << 32 |
                         static_cast<uint64_t>(buffer[pos + 4]) << 24 |
                         static_cast<uint64_t>(buffer[pos + 5]) << 16 |
                         static_cast<uint64_t>(buffer[pos + 6]) << 8 |
                         static_cast<uint64_t>(buffer[pos + 7]);
    pos += 8;
    return ret;
}

uint8_t BinaryReader::de_hexify(const uint8_t nibble) {
    auto val = nibble - '0';
    if (val > 9) {
        val = 10 + nibble - 'a';
    }
    if (val > 9) {
        val = 10 + nibble - 'A';
    }
    return val;
}

uint8_t BinaryReader::read_hex16_uint8() {
    return de_hexify(read_uint8()) << 4 | de_hexify(read_uint8());
}

uint64_t BinaryReader::read_hex16_uint64() {
    if (pos + 16 > buffer_size) {
        pos += 16;
        return 0;
    }
    const uint64_t ret = static_cast<uint64_t>(read_hex16_uint8()) << 56 |
                         static_cast<uint64_t>(read_hex16_uint8()) << 48 |
                         static_cast<uint64_t>(read_hex16_uint8()) << 40 |
                         static_cast<uint64_t>(read_hex16_uint8()) << 32 |
                         static_cast<uint64_t>(read_hex16_uint8()) << 24 |
                         static_cast<uint64_t>(read_hex16_uint8()) << 16 |
                         static_cast<uint64_t>(read_hex16_uint8()) << 8 |
                         static_cast<uint64_t>(read_hex16_uint8());
    return ret;
}

uint16_t BinaryReader::read_remainder_len() {
    return buffer_size - pos;
}

const uint8_t *BinaryReader::read_data(const uint16_t len) {
    if (pos + len > buffer_size) {
        pos += len;
        return nullptr;
    }
    const uint8_t *ret = &buffer[pos];
    pos += len;
    return ret;
}

uint16_t BinaryReader::test_only_current_pos() const {
    return pos;
}
