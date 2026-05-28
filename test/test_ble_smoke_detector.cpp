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

const std::string data_booted = "014207000000000000096b000000aeedd867cf2c00000000000000096b0d0000096b2d0000aeedd867cf2c12536d6f6b654465746563746f723a65646165000d5b303030325d20426f6f746564";
const std::string data_still_alive = "014207000000000000316100000082f2d867cf2c0000000000000031610d000031612d000082f2d867cf2c0d52657065617465723a6632383200125b303030635d205374696c6c20616c697665";
const std::string data_smoke_seen = "014207000000000000320b00000082f2d867cf2c00000000000000320b0d0000320b2d000082f2d867cf2c0d52657065617465723a6632383200115b303030635d20536d6f6b65207365656e";

BleConnectionTracker *connection_tracker_ptr=nullptr;

TEST_CASE("ReadDataBooted", "[booted]") {
    const uint8_t datalen=data_booted.length()/2;
    uint8_t uint_array[datalen];
    populate_array_from_string(uint_array, data_booted);

    BinaryReader reader(0,uint_array,sizeof(uint_array));

    REQUIRE(reader.read_uint8() == 1);//version
    REQUIRE(reader.read_uint8() == 0x42);//type
    REQUIRE(reader.read_uint8() == 7);//ttl
    REQUIRE(reader.read_uint64() == 0x96b);//timestamp
    REQUIRE(reader.read_uint8() == 0);//packet flags
    REQUIRE(reader.read_uint64() == 0xaeedd867cf2c);//sender id
    REQUIRE(reader.read_uint8() == 0);//message flags
    REQUIRE(reader.read_uint64() == 0x96b);//timestamp

    auto message_id_length = reader.read_uint8();
    REQUIRE(message_id_length == 13);
    REQUIRE(reader.read_uint32() == 0x000096b);//message id first third - time
    REQUIRE(reader.read_uint8() == 0x2d);//divider
    REQUIRE(reader.read_uint64() == 0x0000aeedd867cf2c);//message id last two thirds

    auto sender_nick_length = reader.read_uint8();
    REQUIRE(sender_nick_length == 18);
    auto sender_nick = reader.read_data(sender_nick_length);
    auto sender_nick_string = std::string(reinterpret_cast<const char *>(sender_nick), sender_nick_length);
    REQUIRE(sender_nick_string == "SmokeDetector:edae");

    uint16_t content_length = reader.read_uint16();
    REQUIRE(content_length == 0xd);
    auto content = reader.read_data(content_length);
    auto content_string = std::string(reinterpret_cast<const char *>(content), content_length);
    REQUIRE(content_string == "[0002] Booted");
}

TEST_CASE("WriteDataStillAlive", "[still_alive]") {
    std::vector<uint8_t> uint8_target;
    populate_vector_from_string(&uint8_target, data_still_alive);

    std::vector<uint8_t> uint8_vector;
    BinaryWriter writer(uint8_vector);

    writer.write_uint8(1); //version
    writer.write_uint8(0x42); //type
    writer.write_uint8(7);//ttl
    writer.write_uint64(0x3161); //timestamp
    writer.write_uint8(0); //flags
    writer.write_uint64(0x82f2d867cf2c); //sender
    writer.write_uint8(0); //message flags
    writer.write_uint64(0x3161); //message timestamp

    REQUIRE(29 == writer.test_only_current_pos());
    auto eq = std::equal(std::begin(uint8_vector), std::end(uint8_vector), std::begin(uint8_target));
    REQUIRE(true == eq);
}

TEST_CASE("ReadDataSmokeSeen", "[data_smoke_seen]") {
    const uint8_t datalen=data_smoke_seen.length()/2;
    uint8_t uint_array[datalen];
    populate_array_from_string(uint_array, data_smoke_seen);

    BinaryReader reader(0,uint_array,sizeof(uint_array));

    REQUIRE(reader.read_uint8() == 1);//version
    REQUIRE(reader.read_uint8() == 0x42);//type
    REQUIRE(reader.read_uint8() == 7);//ttl
    REQUIRE(reader.read_uint64() == 0x320b);//timestamp
    REQUIRE(reader.read_uint8() == 0);//flags
    REQUIRE(reader.read_uint64() == 0x82f2d867cf2c);//sender
    REQUIRE(reader.read_uint8() == 0);//message flags
    REQUIRE(reader.read_uint64() == 0x320b);//message timestamp

    auto message_id_length = reader.read_uint8();
    REQUIRE(message_id_length == 13);
    REQUIRE(reader.read_uint32() == 0x320b);//message id first third - time
    REQUIRE(reader.read_uint8() == 0x2d);//divider
    REQUIRE(reader.read_uint64() == 0x82f2d867cf2c);//message id last two thirds

    auto sender_nick_length = reader.read_uint8();
    REQUIRE(sender_nick_length == 13);
    auto sender_nick = reader.read_data(sender_nick_length);
    auto sender_nick_string = std::string(reinterpret_cast<const char *>(sender_nick), sender_nick_length);
    REQUIRE(sender_nick_string == "Repeater:f282");

    uint16_t content_length = reader.read_uint16();
    REQUIRE(content_length == 17);
    auto content = reader.read_data(content_length);
    auto content_string = std::string(reinterpret_cast<const char *>(content), content_length);
    REQUIRE(content_string == "[000c] Smoke seen");
}

