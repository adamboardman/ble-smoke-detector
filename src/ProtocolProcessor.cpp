#if defined(PICO_BOARD) || defined(MOCK_PICO_PI)
#include "Debugging.h"
#endif
#include "include/int_types.h"

#include "ProtocolProcessor.h"

#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <string>
#include <cinttypes>

#include "BinaryReader.h"
#include "PacketTypes.h"
#include "Announce.h"
#include "Message.h"

#if defined(PICO_BOARD) || defined(MOCK_PICO_PI)
extern void print_named_data(const char *name, const uint8_t *data, uint16_t data_size);
#endif

const char *ProtocolProcessor::stringForType(const uint8_t type) {
    switch (type) {
        case type_announce:
            return "Announce";
        case type_message:
            return "Message";
        default:
            return "UnknownType";
    }
}

void ProtocolProcessor::updateOrStorePeerNameFromAnnouncement(Announce &announce, BleConnection &connection) const {
    auto &peer = ble_connection_tracker.checkSenderInPeers(announce.getPacketSenderId());
    peer.updateName(announce.getName());
    if (announce.getPacketTtl() >= peer.getAnnounceTtl()) {
        peer.setAnnounceTtl(announce.getPacketTtl());
        ble_connection_tracker.setConnectionHandleForPeer(connection.getConnectionHandle(), &peer);
    }
}

void ProtocolProcessor::processWrite(BleConnection &connection, const uint16_t offset, const uint8_t *buffer,
                                     const uint16_t buffer_size) const {
    BinaryReader reader(offset, buffer, buffer_size);
    const auto version = reader.read_uint8();
    if (version < 1 || version > 2) {
        LOG_DEBUG("Unknown Protocol Version: %d\n", version);
        return;
    }
    const auto type = reader.read_uint8();
    LOG_DEBUG("type: %d (%s)\n", type, stringForType(type));

    switch (type) {
        case type_announce: {
            Announce announce(type, version, reader);
            updateOrStorePeerNameFromAnnouncement(announce, connection);
            ble_connection_tracker.possiblyUpdateTimeOffset(announce.getPacketTimestamp());
            if (const auto stored_announce = ble_connection_tracker.storeAnnounceAndReturnIfNew(announce)) {
                if (const auto newTtl = stored_announce->getPacketTtl()-1; newTtl > 0) {
                    stored_announce->setPacketTtl(newTtl);
                    auto &peer = ble_connection_tracker.checkSenderInPeers(stored_announce->getPacketSenderId());
                    ble_connection_tracker.enqueueBroadcastPacket(stored_announce, &connection, &peer);
                }
            }
            break;
        }
        case type_message: {
            Message message(type, version, reader);
            if (const auto stored_message = ble_connection_tracker.storeMessageAndReturnIfNew(message)) {
                if (const auto newTtl = stored_message->getPacketTtl()-1; newTtl > 0) {
                    stored_message->setPacketTtl(newTtl);
                    auto &peer = ble_connection_tracker.checkSenderInPeers(stored_message->getPacketSenderId());
                    ble_connection_tracker.enqueueBroadcastPacket(stored_message, &connection, &peer);
                }
            }
            break;
        }
        default:
            break;
    }
}
