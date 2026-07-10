#pragma once

enum PacketType {
    type_unknown = 0,
    type_announce = 0x01,
    type_message = 0x02,
    type_bitchat_leave = 0x03,
    type_bitchat_message_old = 0x04,
    type_bitchat_noiseHandshake = 0x10, // Noise handshake initiation
    type_bitchat_noiseEncrypted = 0x11, // Noise encrypted transport message
    type_bitchat_fragment = 0x20, // Fragmentation for large packets
    type_bitchat_request_sync = 0x21,
    type_bitchat_file_transfer = 0x22, // File transfer packet (BLE voice notes, etc.)

    // bump up values to avoid clash with upstream bitchat - if they adopt compatible concepts then we can migrate and use their types
    type_meshtastic_encrypted = 0x7e,
    type_meshtastic = 0x7f
};

enum PacketFlag {
    packet_flag_has_recipient = 1<<0,
    packet_flag_has_signature = 1<<1,
    packet_flag_is_compressed = 1<<2,
    packet_flag_has_route = 1<<3,

    // Bump up to avoid overlap with future bitchat flags
    packet_flag_from_meshtastic = 1<<7
};

//Type, Length, Value
enum AnnounceTLVType {
    tlv_announce_nickname = 0x01,
    tlv_announce_noise_public_key = 0x02,
    tlv_announce_signing_public_key = 0x03,
    tlv_announce_direct_neighbors = 0x04,
    tlv_announce_capabilities = 0x05,
    tlv_announce_bridgeGeohash = 0x06
};

enum MessageTLVType {
    tlv_message_id = 0x00,
    tlv_message_content = 0x01,

    // bump up values to avoid clash with upstream bitchat - if they adopt compatible concepts then we can migrate and use their types
    tlv_message_flags = 0x7b,
    tlv_message_reply_to_id = 0x7c,
    tlv_message_sender_nickname = 0x7d, // it's useful for a channel to have direct access to the name associated with the message
    tlv_message_channel_content = 0x7e, // to avoid regular bitchat clients showing our channel messages to everyone we omit the regular content
    tlv_message_channel = 0x7f // the #channel name
};

enum MessageTypeFlags {
    // bump up to avoid values that might be used if bitchat starts using flags within messages again
    message_flag_emoji = 1<<7,
};

enum SyncTLVType {
    tlv_sync_p = 0x01,
    tlv_sync_m = 0x02,
    tlv_sync_data = 0x03,
    tlv_sync_type_flags = 0x04,
    tlv_sync_since_timestamp = 0x05,
    tlv_sync_fragment_id = 0x06
};

enum SyncTypeFlags {
    flag_sync_announce = 1<<0,
    flag_sync_message = 1<<1,
    flag_sync_leave = 1<<2,
    flag_sync_noise_handshake = 1<<3,
    flag_sync_noise_encrypted = 1<<4,
    flag_sync_fragment = 1<<5,
    flag_sync_request_sync = 1<<6,
    flag_sync_file_transfer = 1<<7,
    // jump to skip over likely bitchat expansion
    flag_sync_meshtastic_encrypted = 1<<30,
    flag_sync_meshtastic = 1<<31
};
