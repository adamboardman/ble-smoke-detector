#pragma once

enum PacketType {
    type_unknown = 0,
    type_announce = 0x01,
    type_message = 0x02,
};

enum PacketFlag {
    packet_flag_has_recipient = 0x01,
    packet_flag_has_signature = 0x02,
    packet_flag_is_compressed = 0x04,
    packet_flag_has_route = 0x08
};

//Type, Length, Value
enum TLVType {
    type_nickname = 0x01,
    type_noise_public_key = 0x02,
    type_signing_public_key = 0x03,
    type_direct_neighbors = 0x04
};
