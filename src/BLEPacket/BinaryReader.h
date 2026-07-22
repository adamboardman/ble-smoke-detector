#ifndef BINARY_READER_H
#define BINARY_READER_H

#include <cstdint>
#include "ble_types.h"

class BinaryReader {
public:
    explicit BinaryReader(const uint16_t offset, const uint8_t *buffer, const uint16_t buffer_size) : offset(offset),
        buffer(buffer), buffer_size(buffer_size), pos(offset) {
    }

    uint8_t read_uint8();

    uint16_t read_uint16();

    int32_t read_int32();

    uint32_t read_uint32();

    uint64_t read_uint64();

    static uint8_t de_hexify(uint8_t nibble);

    uint8_t read_hex16_uint8();

    uint64_t read_hex16_uint64();

    uint16_t read_remainder_len();

    const uint8_t *read_data(uint16_t len);

    [[nodiscard]] uint16_t test_only_current_pos() const;

private:
    const uint16_t offset;
    const uint8_t *buffer;
    const uint16_t buffer_size;
    uint16_t pos;
};

#endif
