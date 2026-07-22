#ifndef PROTOCOL_PROCESSOR_H
#define PROTOCOL_PROCESSOR_H
#include <vector>

#include "Base.h"
#include "Announce.h"
#include "BleConnection.h"
#include "BleConnectionTracker.h"

class ProtocolProcessor {
public:
    explicit ProtocolProcessor(BleConnectionTracker &ble_connection_tracker)
        : ble_connection_tracker(ble_connection_tracker) {
    }

    static const char *stringForType(uint8_t type);

    void updateOrStorePeerNameFromAnnouncement(Announce &announce, BleConnection *connection) const;

    void processWrite(BleConnection *connection, uint16_t offset, const uint8_t *buffer, uint16_t buffer_size) const;

private:
    BleConnectionTracker &ble_connection_tracker;
};

#endif