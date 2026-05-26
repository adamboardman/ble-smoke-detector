#include <catch2/catch_test_macros.hpp>
#include <cstdarg>
#include <cstdint>
#include <cstring>
#include <vector>

#include "Debugging.h"
#include "pico_pi_mocks.h"
#include "../Packet/BinaryReader.h"
#include "../Packet/BinaryWriter.h"
#include "../BLE/BleConnectionTracker.h"

extern void print_named_data(const char *name, const uint8_t *data, uint16_t data_size);

const std::string data_booted = "0102070000000000000944000000b86012c1cd2800370000000000000009440c000009440000b86012c1cd2811536d6f6b654465746563746f7236306238000d5b303030325d20426f6f746564";
const std::string data_still_alive = "0102070000000000003b44000000b86012c1cd28003c000000000000003b440c00003b440000b86012c1cd2811536d6f6b654465746563746f723630623800125b303030665d205374696c6c20616c697665";
const std::string data_smoke_seen = "";

BleConnectionTracker *connection_tracker_ptr=nullptr;

TEST_CASE("ReadDataBooted", "[booted]") {
    const uint8_t datalen=data_booted.length()/2;
    uint8_t uint_array[datalen];
    populate_array_from_string(uint_array, data_booted);

    BinaryReader reader(0,uint_array,sizeof(uint_array));

    REQUIRE(reader.read_uint8() == 1);//version
    REQUIRE(reader.read_uint8() == 2);//type
    REQUIRE(reader.read_uint8() == 7);//ttl
    REQUIRE(reader.read_uint64() == 0x944);//timestamp
    REQUIRE(reader.read_uint8() == 0);//packet flags
    REQUIRE(reader.read_uint64() == 0xb86012c1cd28);//sender id
    auto payloadlength = reader.read_uint16();
    REQUIRE(payloadlength == 55);//payloadlength
    REQUIRE(reader.read_uint8() == 0);//message flags
    REQUIRE(reader.read_uint64() == 0x944);//timestamp

    auto message_id_length = reader.read_uint8();
    REQUIRE(message_id_length == 12);
    REQUIRE(reader.read_uint64() == 0x00009440000b860);//message id first two thirds
    REQUIRE(reader.read_uint32() == 0x12c1cd28);//message id last third

    auto sender_nick_length = reader.read_uint8();
    REQUIRE(sender_nick_length == 17);
    auto sender_nick = reader.read_data(sender_nick_length);
    auto sender_nick_string = std::string(reinterpret_cast<const char *>(sender_nick), sender_nick_length);
    REQUIRE(sender_nick_string == "SmokeDetector60b8");

    uint16_t content_length = reader.read_uint16();
    REQUIRE(content_length == 0xd);
    auto content = reader.read_data(content_length);
    auto content_string = std::string(reinterpret_cast<const char *>(content), content_length);
    REQUIRE(content_string == "[0002] Booted");
}

TEST_CASE("WriteDataStillAlive", "[still_alive]") {
    std::vector<uint8_t> uint8_vector;
    BinaryWriter writer(uint8_vector);

    writer.write_uint8(1); //version
    writer.write_uint8(1); //type
    writer.write_uint8(3);//ttl
    writer.write_uint64(0x1987183cdf9); //timestamp
    writer.write_uint8(0); //flags
    uint8_t payload[] = {'a', 'd', 'a', 'm'};
    writer.write_uint16(sizeof(payload));
    writer.write_uint64(0x1d3d6a261523a828); //sender
    writer.write_data(payload, sizeof(payload));
    writer.write_uint8(0); //no tail clipping

    REQUIRE(27 == writer.test_only_current_pos());
    auto eq = std::equal(std::begin(uint8_vector), std::end(uint8_vector), std::begin(data_still_alive));
    REQUIRE(true == eq);
}

TEST_CASE("ReadDataSmokeSeen", "[data_smoke_seen]") {
    const uint8_t datalen=data_smoke_seen.length()/2;
    uint8_t uint_array[datalen];
    populate_array_from_string(uint_array, data_smoke_seen);

    BinaryReader reader(0,uint_array,sizeof(uint_array));

    REQUIRE(reader.read_uint8() == 1);//version
    REQUIRE(reader.read_uint8() == 1);//type
    REQUIRE(reader.read_uint8() == 3);//ttl
    REQUIRE(reader.read_uint64() == 0x198717c1114);//timestamp
    uint8_t flags = reader.read_uint8();
    REQUIRE(flags == 0);//flags
    auto payloadlength = reader.read_uint16();
    REQUIRE(payloadlength == 4);//payloadlength
    REQUIRE(reader.read_uint64() == 0x19077f0222faf5ce);//sender
    if (flags & 0b1) { // has recipient
        REQUIRE(reader.read_uint64() == 0x00f00f00f00f);//recipient
    }
    if (flags & 0b100) {
        //compressed gives original size before payload
        const auto originalSize = reader.read_uint16();
        LOG_DEBUG("originalSize: %d\n", originalSize);
    }
    uint8_t payloadexpected[] = {'a', 'd', 'a', 'm'};
    uint8_t payload[payloadlength];
    memcpy (payload, reader.read_data(payloadlength),payloadlength);

    REQUIRE(payload[0] == payloadexpected[0]);
    REQUIRE(payload[1] == payloadexpected[1]);
    REQUIRE(payload[2] == payloadexpected[2]);
    REQUIRE(payload[3] == payloadexpected[3]);
    if (flags & 0b10) { // has signature
        uint8_t signature_length = 64;
        uint8_t signature[signature_length];
        memcpy (signature, reader.read_data(signature_length),signature_length);
        uint8_t signatureexpected[] = {'n','o','n','e'};
        REQUIRE(signature[0] == signatureexpected[0]);
    }

    auto padding_to_remove = uint_array[datalen-1];
    REQUIRE(0x9a == padding_to_remove);

    // For some reason our test captured packets appear to be having data after the payload
    print_named_data("packet data", &uint_array[reader.test_only_current_pos()], datalen-padding_to_remove);

}

