#include <catch2/catch_test_macros.hpp>
#include <cstdarg>
#include <cstdint>
#include <cstring>
#include <vector>

#include "Debugging.h"
#include "pico_pi_mocks.h"
#include "BinaryReader.h"
#include "BinaryWriter.h"
#include "BleConnectionTracker.h"

const std::string data_booted = "0102070000019f4c69f4000000330000724860b2f180000d4c69f4003f0000724860b2f1807d12536d6f6b654465746563746f723a343837327f0623736d6f6b657e06426f6f74656400";
const std::string data_booted_smoke_seen = "0102070000019f4c6bc8c000003e0000724860b2f180000d4c6bc8c0a70000724860b2f1807d12536d6f6b654465746563746f723a343837327f0623736d6f6b657e11426f6f7465642b536d6f6b65207365656e00";

TEST_CASE("ReadDataBooted", "[booted]") {
    const uint8_t datalen=data_booted.length()/2;
    uint8_t uint_array[datalen];
    populate_array_from_string(uint_array, data_booted);

    BinaryReader reader(0,uint_array,sizeof(uint_array));

    REQUIRE(reader.read_uint8() == 1); //version
    REQUIRE(reader.read_uint8() == 2); //type
    REQUIRE(reader.read_uint8() == 7); //ttl
    REQUIRE(reader.read_uint64() == 0x19f4c69f400); //timestamp
    REQUIRE(reader.read_uint8() == 0); //packet flags
    REQUIRE(reader.read_uint16() == 51); //payload length
    REQUIRE(reader.read_uint64() == 0x724860b2f180); //sender id

    //Message
    REQUIRE(reader.read_uint8() == tlv_message_id);
    auto messageIdLength = reader.read_uint8();
    REQUIRE(messageIdLength == 13);
    const uint8_t *mesIdRead = reader.read_data(messageIdLength);
    uint8_t mesIdExpected[] = {76,105,0xF4,0,63,0,0,114,72,96,178,241,128};
    REQUIRE(mesIdRead[0] == mesIdExpected[0]);
    REQUIRE(mesIdRead[3] == mesIdExpected[3]);
    REQUIRE(mesIdRead[4] == mesIdExpected[4]);
    REQUIRE(mesIdRead[5] == mesIdExpected[5]);
    REQUIRE(mesIdRead[6] == mesIdExpected[6]);
    REQUIRE(mesIdRead[7] == mesIdExpected[7]);
    REQUIRE(mesIdRead[8] == mesIdExpected[8]);
    REQUIRE(mesIdRead[9] == mesIdExpected[9]);
    REQUIRE(mesIdRead[10] == mesIdExpected[10]);
    REQUIRE(mesIdRead[11] == mesIdExpected[11]);
    REQUIRE(mesIdRead[12] == mesIdExpected[12]);

    REQUIRE(reader.read_uint8() == tlv_message_sender_nickname);
    auto senderNickLength = reader.read_uint8();
    REQUIRE(senderNickLength == 18);
    std::string senderNick(reinterpret_cast<const char *>(reader.read_data(senderNickLength)), senderNickLength);
    REQUIRE(senderNick == "SmokeDetector:4872");

    REQUIRE(reader.read_uint8() == tlv_message_channel);
    auto channelLength = reader.read_uint8();
    REQUIRE(channelLength == 6);
    std::string channel(reinterpret_cast<const char *>(reader.read_data(channelLength)), channelLength);
    REQUIRE(channel == "#smoke");

    REQUIRE(reader.read_uint8() == tlv_message_channel_content);
    auto payloadLength = reader.read_uint8();
    REQUIRE(payloadLength == 6);
    std::string content(reinterpret_cast<const char *>(reader.read_data(payloadLength)), payloadLength);
    REQUIRE(content == "Booted");
}

TEST_CASE("WriteDataStillAlive", "[still_alive]") {
    const uint8_t datalen=data_booted_smoke_seen.length()/2;
    uint8_t uint_array[datalen];
    populate_array_from_string(uint_array, data_booted_smoke_seen);

    BinaryReader reader(0,uint_array,sizeof(uint_array));

    REQUIRE(reader.read_uint8() == 1); //version
    REQUIRE(reader.read_uint8() == 2); //type
    REQUIRE(reader.read_uint8() == 7); //ttl
    REQUIRE(reader.read_uint64() == 0x19f4c6bc8c0); //timestamp
    REQUIRE(reader.read_uint8() == 0); //packet flags
    REQUIRE(reader.read_uint16() == 62); //payload length
    REQUIRE(reader.read_uint64() == 0x724860b2f180); //sender id

    //Message
    REQUIRE(reader.read_uint8() == tlv_message_id);
    auto messageIdLength = reader.read_uint8();
    REQUIRE(messageIdLength == 13);
    const uint8_t *mesIdRead = reader.read_data(messageIdLength);
    uint8_t mesIdExpected[] = {76,107,200,192,167,0,0,114,72,96,178,241,128};
    REQUIRE(mesIdRead[0] == mesIdExpected[0]);
    REQUIRE((int)(mesIdRead[1]) == mesIdExpected[1]);
    REQUIRE((int)mesIdRead[2] == mesIdExpected[2]);
    REQUIRE((int)mesIdRead[3] == mesIdExpected[3]);
    REQUIRE((int)mesIdRead[4] == mesIdExpected[4]);
    REQUIRE(mesIdRead[5] == mesIdExpected[5]);
    REQUIRE(mesIdRead[6] == mesIdExpected[6]);
    REQUIRE(mesIdRead[7] == mesIdExpected[7]);
    REQUIRE(mesIdRead[8] == mesIdExpected[8]);
    REQUIRE(mesIdRead[9] == mesIdExpected[9]);
    REQUIRE(mesIdRead[10] == mesIdExpected[10]);
    REQUIRE(mesIdRead[11] == mesIdExpected[11]);
    REQUIRE(mesIdRead[12] == mesIdExpected[12]);

    REQUIRE(reader.read_uint8() == tlv_message_sender_nickname);
    auto senderNickLength = reader.read_uint8();
    REQUIRE(senderNickLength == 18);
    std::string senderNick(reinterpret_cast<const char *>(reader.read_data(senderNickLength)), senderNickLength);
    REQUIRE(senderNick == "SmokeDetector:4872");

    REQUIRE(reader.read_uint8() == tlv_message_channel);
    auto channelLength = reader.read_uint8();
    REQUIRE(channelLength == 6);
    std::string channel(reinterpret_cast<const char *>(reader.read_data(channelLength)), channelLength);
    REQUIRE(channel == "#smoke");

    REQUIRE(reader.read_uint8() == tlv_message_channel_content);
    auto payloadLength = reader.read_uint8();
    REQUIRE(payloadLength == 17);
    std::string content(reinterpret_cast<const char *>(reader.read_data(payloadLength)), payloadLength);
    REQUIRE(content == "Booted+Smoke seen");
}
