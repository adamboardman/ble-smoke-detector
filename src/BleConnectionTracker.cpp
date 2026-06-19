#include <algorithm>
#include <cstdlib>
#include <limits>
#include <malloc.h>
#include <ranges>

#include "BleConnectionTracker.h"

#include "include/int_types.h"
#include "Announce.h"
#include "BinaryWriter.h"

#ifdef MOCK_PICO_PI
#include "Debugging.h"
#include "../test/packet_repeater_mocks.h"
#include "../test/pico_pi_mocks.h"
#else
#ifdef PICO_BOARD
#include "Debugging.h"
#include "pico/time.h"
#include "hardware/timer.h"
#include "ble/att_server.h"
#else
#include "include/ble_types.h"
#endif
#endif

extern BleConnectionTracker *connection_tracker_ptr;
static btstack_context_callback_registration_t notify_context_callback_registration;
static btstack_context_callback_registration_t write_context_callback_registration;

void packet_can_send_notification_handler(void *context) {
    LOG_DEBUG("packet_can_send_notification_handler(0x%p)\n", context);
#if __WORDSIZE == 64
    hci_con_handle_t con_handle = reinterpret_cast<uint64_t>(context) & 0xffff;
#else
    auto con_handle = reinterpret_cast<uint32_t>(context);
#endif
    if (connection_tracker_ptr)
        connection_tracker_ptr->notifyRawPacket(con_handle);
}

void packet_can_write_without_response_handler(void *context) {
    LOG_DEBUG("packet_can_write_without_response_handler(0x%2p)\n", context);
#if __WORDSIZE == 64
    hci_con_handle_t con_handle = reinterpret_cast<uint64_t>(context) & 0xffff;
#else
    auto con_handle = reinterpret_cast<uint32_t>(context);
#endif
    if (connection_tracker_ptr)
        connection_tracker_ptr->writeRawPacket(con_handle);
}

BleConnection &BleConnectionTracker::connectionForConnHandle(const hci_con_handle_t connection_handle) {
    if (const auto search = connections.find(connection_handle); search != connections.end()) {
        return connections[connection_handle];
    }
    connections[connection_handle].setConnectionHandle(connection_handle);
    return connections[connection_handle];
}

Message *BleConnectionTracker::storeMessageAndReturnIfNew(const Message &message) {
    const auto &id = message.getMessageId();
    if (const auto search = messages.find(id); search != messages.end()) {
        return nullptr; //message was found so it not new
    }
    messages[id] = message;
    return &messages[id];
}

Announce *BleConnectionTracker::storeAnnounceAndReturnIfNew(Announce &ann) {
    const auto id = ann.getPacketHash();
    if (const auto search = announces.find(id); search != announces.end()) {
        return nullptr; //announce was found so it not new
    }
    announces[id] = std::move(ann);
    return &announces[id];
}

Message *BleConnectionTracker::messageWithId(const std::string &id) {
    if (const auto search = messages.find(id); search != messages.end()) {
        return &search->second;
    }
    return nullptr;
}

Peer *BleConnectionTracker::peerWithId(const uint64_t id) {
    if (const auto search = peers.find(id); search != peers.end()) {
        return &search->second;
    }
    return nullptr;
}

Peer &BleConnectionTracker::checkSenderInPeers(const uint64_t sender) {
    if (const auto search = peers.find(sender); search == peers.end()) {
        peers[sender].setId(sender);
    }
    return peers[sender];
}

void BleConnectionTracker::enqueueBroadcastPacket(Base *packet) {
    if (packet->getPacketTtl() > 0) {
        broadcast_packets_to_send_list.push_back(packet);
    }
}

void BleConnectionTracker::enqueueBroadcastPacket(Base *packet, BleConnection *from_connection,
                                                  Peer *from_peer) {
    packets_connections_sent_list.emplace(packet, from_connection);
    packets_peers_sent_list.emplace(packet, from_peer);
    enqueueBroadcastPacket(packet);
}

void BleConnectionTracker::updateRssiForActiveConnections(const uint8_t *bt_address, const int8_t rssi) {
    // Update RSSI values for any active connections
    for (auto &connection: connections | std::views::values) {
        if (memcmp(connection.getAddress(), bt_address, BD_ADDR_LEN) == 0) {
            connection.setRssi(rssi);
        }
    }
}

void BleConnectionTracker::addAvailablePeer(const uint8_t *bt_address, const uint8_t bt_address_type,
                                            const service_uuid_check_status services, const int8_t rssi,
                                            const bool use_coded) {
    const auto address = std::string(reinterpret_cast<const char *>(bt_address), BD_ADDR_LEN);
    available_neighbours[address].setBleAddress(bt_address, bt_address_type);
    available_neighbours[address].setServices(services);
    available_neighbours[address].setRssi(rssi);
    available_neighbours[address].setTimestamp(time_us_64());
    available_neighbours[address].setUseCodedPhy(use_coded);

    updateRssiForActiveConnections(bt_address, rssi);
}

void BleConnectionTracker::addAvailablePeer(const bd_addr_t &bt_address, const bd_addr_type_t bt_address_type,
                                            const service_uuid_check_status services, const int8_t rssi,
                                            const bool use_coded) {
    const auto address = std::string(reinterpret_cast<const char *>(bt_address), BD_ADDR_LEN);
    available_neighbours[address].setBleAddress(bt_address, bt_address_type);
    available_neighbours[address].setServices(services);
    available_neighbours[address].setRssi(rssi);
    available_neighbours[address].setTimestamp(time_us_64());
    available_neighbours[address].setUseCodedPhy(use_coded);

    updateRssiForActiveConnections(bt_address, rssi);
}

BleConnection &BleConnectionTracker::reportConnection(const uint16_t handle, const bd_addr_t &addr,
                                                      const bd_addr_type_t address_type) {
    const auto address = std::string(reinterpret_cast<const char *>(addr), BD_ADDR_LEN);
    if (const auto item = available_neighbours.find(address); item != available_neighbours.end()) {
        connections[handle] = item->second;
        available_neighbours.erase(address);
    }
    connections[handle].setConnectionHandle(handle);
    connections[handle].setConnected(true);
    connections[handle].setBleAddress(addr, address_type);
    connections[handle].setTimestamp(time_us_64());
    available_neighbours.erase(address);
    return connections[handle];
}

#if defined(PICO_BOARD) || defined(MOCK_PICO_PI)
void handle_gatt_client_value_update_event(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
#endif

BleConnection &BleConnectionTracker::reportConnection(const uint16_t handle, const bd_addr_t &addr,
                                                      const bd_addr_type_t address_type, const uint8_t role) {
    auto &connection = reportConnection(handle, addr, address_type);
    connection.setRole(role);

#if defined(PICO_BOARD) || defined(MOCK_PICO_PI)
    if (role == HCI_ROLE_MASTER) {
        gatt_client_characteristic_t le_characteristic;
        //gatt_client_listen_for_characteristic_value_updates only reads the value_handle
        le_characteristic.value_handle = connection.getPacketCharacteristicValueHandle();
        gatt_client_listen_for_characteristic_value_updates(connection.getNotificationListener(),
                                                            handle_gatt_client_value_update_event, handle,
                                                            &le_characteristic);
    }
#endif

    return connection;
}

void BleConnectionTracker::reportConnectionFailure(const bd_addr_t &addr, int reason) {
    const auto address = std::string(reinterpret_cast<const char *>(addr), BD_ADDR_LEN);
    if (const auto item = available_neighbours.find(address); item != available_neighbours.end()) {
        item->second.setConnectFailure(reason);
    }
}

void BleConnectionTracker::reportDisconnection(const uint16_t handle) {
    auto &removed_connection = connections[handle];
    removed_connection.setNotificationEnabled(false);
    removed_connection.setConnected(false);
    auto &removed_peer = peers[handle];

#if defined(PICO_BOARD) || defined(MOCK_PICO_PI)
    if (removed_connection.getRole() == HCI_ROLE_MASTER) {
        gatt_client_stop_listening_for_characteristic_value_updates(removed_connection.getNotificationListener());
    }
#endif
    auto announced_to_connection = [this, handle](const auto &item) {
        const auto &[packet, connection] = item;

        const Announce *packetAsAnnounce = nullptr;
        if (packet->getPacketType() == type_announce) {
            packetAsAnnounce = static_cast<Announce *>(packet);
            //LOG_DEBUG("disconnection - checking 0x%" PRIx64 ", 0x%" PRIx64 ", %d, %d\n",
            //          packetAsAnnounce->getPacketSenderId(), announce.getPacketSenderId(),
            //          connection->getConnectionHandle(), handle);
        }
        return packetAsAnnounce != nullptr &&
               connection->getConnectionHandle() == handle;
    };
    const auto handle_peers_removed = handle_peer_map.erase(handle);
    const auto announce_p_c_removed = std::erase_if(packets_connections_sent_list, announced_to_connection);
    auto announced_to_peer = [this, removed_peer](const auto &item) {
        const auto &[packet, peer] = item;

        const Announce *packetAsAnnounce = nullptr;
        if (packet->getPacketType() == type_announce) {
            packetAsAnnounce = static_cast<Announce *>(packet);
        }

        return packetAsAnnounce != nullptr &&
               peer == &removed_peer;
    };
    const auto announce_p_s_removed = std::erase_if(packets_peers_sent_list, announced_to_peer);
    LOG_DEBUG("disconnection - removed announce packets: (%lu, %lu), handle_peers_removed: %lu\n",
              announce_p_c_removed, announce_p_s_removed, handle_peers_removed);
}

bool BleConnectionTracker::weHaveTheTime() const {
    return timestamp_offset_ms >= build_time_ms;
}

std::vector<BleConnection *> BleConnectionTracker::getConnectableNeighbours() {
    std::vector<BleConnection *> neighbours;
    const auto now = time_us_64();
    for (auto &connection: available_neighbours | std::views::values) {
        if ((weHaveTheTime() || connection.isRepeater()) && connection.isConnected() == false && !
            connection.isRandom() && connection.getFailReason() == 0) {
            neighbours.push_back(&connection);
        }
    }
    return neighbours;
}

bool BleConnectionTracker::requestNextRssi(const bool restart) {
    static auto iterator = connections.begin();
    if (iterator == connections.end() || restart) {
        iterator = connections.begin();
    }
    auto [handle, connection] = *iterator;
#if defined(PICO_BOARD) || defined(MOCK_PICO_PI)
    gap_read_rssi(handle); //requested and will call back
#endif
    ++iterator;
    return (iterator != connections.end());
}

#if defined(PICO_BOARD) || defined(MOCK_PICO_PI)
extern "C" char __StackLimit; // NOLINT(*-reserved-identifier)
extern "C" char __bss_end__; // NOLINT(*-reserved-identifier)
#endif

void BleConnectionTracker::printTime() {
    const uint64_t time_ms = getTimeMs();
    const uint64_t seconds = time_ms / 1000u;
    const uint64_t minutes = seconds / 60;
    const uint64_t hours = minutes / 60;
    const uint64_t days = hours / 24;

    const uint16_t p_ms = time_ms - (seconds * 1000u);
    const uint16_t p_seconds = seconds - (minutes * 60);
    const uint16_t p_minutes = minutes - (hours * 60u);
    const uint16_t p_hours = hours - (days * 24u);
    LOG_DEBUG("[%02u:%02u:%02u.%03u]", p_hours, p_minutes, p_seconds, p_ms);
}

int BleConnectionTracker::getActiveConnectionCount(bool log) {
    auto active_connections_count = 0;
    for (auto &connection: connections | std::views::values) {
        if (connection.isConnected()) {
            if (log) {
                LOG_DEBUG("{0x%x:%d-%s} ", connection.getConnectionHandle(), connection.getRole(),
                          bd_addr_to_str(connection.getAddress()));
            }
            active_connections_count++;
        }
    }
    return active_connections_count;
}

void BleConnectionTracker::printStats() {
#if defined(PICO_BOARD) || defined(MOCK_PICO_PI)
    const uint32_t total_heap = &__StackLimit - &__bss_end__;
    const uint32_t used_heap = mallinfo().uordblks;
    const uint32_t free_heap = total_heap - used_heap;
#endif

    printTime();
#if defined(PICO_BOARD) || defined(MOCK_PICO_PI)
    LOG_DEBUG(" %d %" PRIu32 ":%" PRIu32 " ", weHaveTheTime(), used_heap, free_heap);
#endif

    int active_connections_count = getActiveConnectionCount(true);
    LOG_DEBUG(
        "con: %d/%zu, avail: %zu, announces:%zu, messages:%zu, broadcast: %zu\n",
        active_connections_count, connections.size(), available_neighbours.size(), announces.size(), messages.size(),
        broadcast_packets_to_send_list.size());
}

#pragma GCC push_options
#pragma GCC optimize ("O0")

bool BleConnectionTracker::SendPacketToConnection(Base &packet, BleConnection &ble_connection) {
    std::vector<uint8_t> packet_data;
    packet.writePacket(packet_data);

    if (ble_connection.getMtu() && packet_data.size() > ble_connection.getMtu()) {
        LOG_DEBUG("SendPacketToConnection packet to big %lu, %d", packet_data.size(), ble_connection.getMtu());
        //TODO - implement fragment creation
        return false;
    }

    const uint16_t con_handle = ble_connection.getConnectionHandle();
    std::string peer_string{};
    uint64_t sender_id = 0;
    const auto peer = peerWithConnectionHandle(con_handle);
    if (peer && !peer->getName().empty()) {
        peer_string.append(peer->getName());
    }
    if (peer) {
        sender_id = peer->getId();
    }
#if defined(PICO_BOARD) || defined(MOCK_PICO_PI)
    hci_connection_t *hci_connection = hci_connection_for_handle(con_handle);

    LOG_DEBUG("SendPacketToConnection - type(%d), peer(%s:0x%" PRIx64 "), hci_connection_for_handle(0x%x), hc(0x%zx)\n",
              packet.getPacketType(), peer_string.c_str(), sender_id,
              con_handle, hci_connection);
    uint8_t status = 0;
    ble_connection.setHasData(true);
    if (ble_connection.getRole() == HCI_ROLE_SLAVE) {
        raw_packet_to_notify.emplace(con_handle, packet_data);
        notify_context_callback_registration.callback = &packet_can_send_notification_handler;
        notify_context_callback_registration.context = reinterpret_cast<void *>(con_handle);
        status = att_server_request_to_send_notification(&notify_context_callback_registration, con_handle);
    } else {
        raw_packet_to_write.emplace(con_handle, packet_data);
        write_context_callback_registration.callback = &packet_can_write_without_response_handler;
        write_context_callback_registration.context = reinterpret_cast<void *>(con_handle);
        status = gatt_client_request_to_write_without_response(&write_context_callback_registration, con_handle);
    }
    if (status) {
        LOG_DEBUG("SendPacketToConnection - Write without response failed, status 0x%02x.\n", status);
        sleep_ms(20);
        return false;
    }
    sleep_ms(20);
    return true;
#elifdef Arduino_h

    NimBLERemoteService *pSvc = nullptr;
    NimBLERemoteCharacteristic *pChr = nullptr;

    auto pClient = NimBLEDevice::getClientByHandle(con_handle);
    if (pClient) {
        pSvc = pClient->getService(serviceUUID);
        if (pSvc) {
            pChr = pSvc->getCharacteristic(charUUID);
        }
        if (pChr) {
            if (pChr->canWrite()) {
                if (pChr->writeValue(packet_data)) {
                    LOG_DEBUG("SendPacketToConnection - wrote value - type(%d), con_handle(0x%x)\n",
                              packet.getPacketType(), con_handle);
                    return true;
                }
            }
        }
    } else {
        //We have been connected to so we need to notify our subscribers
        auto lSvc = NimBLEDevice::getServer()->getServiceByUUID(serviceUUID);
        if (lSvc) {
            auto lChr = lSvc->getCharacteristic(charUUID);
            if (lChr) {
                if (lChr->notify(packet_data.data(), packet_data.size(), con_handle)) {
                    LOG_DEBUG("SendPacketToConnection - notified our characteristic - type(%d), con_handle(0x%x)\n",
                              packet.getPacketType(), con_handle);
                    return true;
                }
            }
        }
    }

    LOG_DEBUG("SendPacketToConnection - Write without response failedtype(%d), con_handle(0x%x)\n",
              packet.getPacketType(), con_handle);
    return false;
#endif
}

bool BleConnectionTracker::havePacketsToSend() {
    return (!broadcast_packets_to_send_list.empty());
}

bool BleConnectionTracker::sendPackets() {
    if (!havePacketsToSend()) {
        return false;
    }
    auto available = [](const BleConnection &connection) {
        return connection.isConnected()
#if defined(PICO_BOARD) || defined(MOCK_PICO_PI)
               && (connection.getPacketCharacteristicValueHandle() > 0 || connection.getRole() == HCI_ROLE_SLAVE);
#else
        ;
#endif
    };
    auto available_connections = connections | std::views::values | std::views::filter(available);

    auto packet_needed = [this](const std::pair<Base *, BleConnection *> &targeted) {
        auto [begin, end] = packets_connections_sent_list.equal_range(targeted.first);
        auto matches_connection = [targeted](const std::pair<Base *, BleConnection *> &sent_to) {
            return sent_to.second->getConnectionHandle() == targeted.second->getConnectionHandle();
        };
        return std::ranges::find_if(begin, end, matches_connection) == end;
    };

    std::set<Base *> broadcast_packets_to_remove;
    for (auto packet: broadcast_packets_to_send_list) {
        bool connections_exist = false;
        LOG_DEBUG("Checking Broadcast Packet Sendable %p\n", packet);
        for (auto &connection: available_connections) {
            bool sendable = true;
            for (auto search = packets_connections_sent_list.find(packet);
                 search != packets_connections_sent_list.end(); ++search) {
                if (search->second->getConnectionHandle() == connection.getConnectionHandle()) {
                    // LOG_DEBUG("Not Sendable 0x%x\n", connection.getConnectionHandle());
                    sendable = false;
                }
            }
            if (sendable) {
                SendPacketToConnection(*packet, connection);
                packets_connections_sent_list.emplace(packet, &connection);
            }
            connections_exist = true;
        }

        if (connections_exist) {
            broadcast_packets_to_remove.emplace(packet);
        }
    }
    auto sent = [broadcast_packets_to_remove](Base *packet) {
        return broadcast_packets_to_remove.contains(packet);
    };
    const auto ret = std::ranges::remove_if(broadcast_packets_to_send_list, sent);
    broadcast_packets_to_send_list.erase(ret.begin(), ret.end());
    return true;
}
#pragma GCC pop_options

constexpr uint64_t MESSAGE_TIMEOUT = 300000L; //5mins in ms

void BleConnectionTracker::possiblyUpdateTimeOffset(const uint64_t timestamp_ms) {
    const uint64_t our_now = getTimeMs();
    const int64_t diff = static_cast<int64_t>(timestamp_ms) - static_cast<int64_t>(our_now);
    const auto diff_abs = llabs(diff);
    if (diff_abs > MESSAGE_TIMEOUT || timestamp_ms > our_now) {
        timestamp_offset_ms += diff;
    }
}

uint64_t BleConnectionTracker::getTimeMs() const {
    return timestamp_offset_ms + (time_us_64() / 1000); //time in ms
}

void BleConnectionTracker::setConnectionStarted(const BleConnection *neighbour) {
    const auto address = std::string(reinterpret_cast<const char *>(neighbour->getAddress()), BD_ADDR_LEN);
    available_neighbours.erase(address);
}

Announce *BleConnectionTracker::getAnyAnnounce() {
    if (!announces.empty()) {
        return &announces.begin()->second;
    }
    return nullptr;
}

void BleConnectionTracker::cleanupStaleItems() {
    auto now = getTimeMs();

    auto packet_connection_stale = [now](const auto &item) {
        const auto &[packet, connection] = item;
        return packet->getPacketTimestamp() + ten_minutes_in_ms < now ||
               !connection->isConnected() && connection->getTimestampMs() + ten_minutes_in_ms < now;
    };
    const auto packets_connections_removed = std::erase_if(packets_connections_sent_list, packet_connection_stale);
    auto packet_peer_stale = [now](const auto &item) {
        const auto &[packet, peer] = item;
        return packet->getPacketTimestamp() + ten_minutes_in_ms < now;
    };
    const auto packets_peers_sent_list_removed = std::erase_if(packets_peers_sent_list, packet_peer_stale);
    auto packet_stale = [now](const auto &packet) {
        return packet->getPacketTimestamp() + ten_minutes_in_ms < now;
    };
    const auto broadcast_packets_removed = std::erase_if(broadcast_packets_to_send_list, packet_stale);
    auto connection_stale = [now](const auto &item) {
        const auto &[key, connection] = item;
        return !connection.isConnected() && connection.getTimestampMs() + ten_minutes_in_ms < now;
    };
    const auto connections_removed = std::erase_if(connections, connection_stale);
    const auto available_neighbours_removed = std::erase_if(available_neighbours, connection_stale);
    const auto messages_removed = std::erase_if(messages, [now](const auto &item) {
        const auto &[key, message] = item;
        return message.getPacketTimestamp() + ten_minutes_in_ms < now;
    });
    const auto announces_removed = std::erase_if(announces, [now](const auto &item) {
        const auto &[key, ann] = item;
        return ann.getPacketTimestamp() + ten_minutes_in_ms < now;
    });

    LOG_DEBUG(
        "Cleanup items removed: connections(%zu), neighbours(%zu), announces(%zu), messages(%zu), packet con(%zu), packet peer(%zu), broadcast(%zu)\n",
        connections_removed, available_neighbours_removed, announces_removed, messages_removed,
        packets_connections_removed, packets_peers_sent_list_removed, broadcast_packets_removed);
}

size_t BleConnectionTracker::getConnectionsCount() const {
    return connections.size() + available_neighbours.size();
}

void BleConnectionTracker::setConnectionHandleForPeer(const uint16_t con_handle, Peer *peer) {
    handle_peer_map[con_handle] = peer;
}

Peer *BleConnectionTracker::peerWithConnectionHandle(const uint16_t con_handle) {
    if (const auto search = handle_peer_map.find(con_handle); search != handle_peer_map.end()) {
        return handle_peer_map[con_handle];
    }
    return nullptr;
}

BleConnection *BleConnectionTracker::getConnectionForAddress(const uint8_t *address) {
    auto search = [&](const BleConnection &connection) {
        return connection.isConnected() && memcmp(connection.getAddress(), address, BD_ADDR_LEN) == 0;
    };
    if (auto found = connections | std::views::values | std::views::filter(search); !found.empty()) {
        return &*found.begin();
    }
    return nullptr;
}

void BleConnectionTracker::notifyRawPacket(const hci_con_handle_t con_handle) {
    auto packets_for_handle = raw_packet_to_notify.equal_range(con_handle);
    if (packets_for_handle.first != raw_packet_to_notify.end()) {
        const auto connection = connections[con_handle];
        ASSERT_DEBUG(connection.hasData());
        const auto &data = packets_for_handle.first->second;
#if defined(PICO_BOARD) || defined(MOCK_PICO_PI)
        const auto ret = att_server_notify(con_handle, connection.getPacketCharacteristicValueHandle(), data.data(),
                                           data.size());
        LOG_DEBUG("notifyRawPacket(0x%x) ret:%d\n", con_handle, ret);
        if (ret == ERROR_CODE_SUCCESS || ret == ERROR_CODE_UNKNOWN_CONNECTION_IDENTIFIER) {
            raw_packet_to_notify.erase(packets_for_handle.first);
        }
#endif
    }
    packets_for_handle = raw_packet_to_notify.equal_range(con_handle);
    if (packets_for_handle.first == raw_packet_to_notify.end()) {
        auto connection = connections[con_handle];
        LOG_DEBUG("notifyRawPacket(0x%x) no more data\n", con_handle);
        connection.setHasData(false);
    } else {
        notify_context_callback_registration.callback = &packet_can_send_notification_handler;
        notify_context_callback_registration.context = reinterpret_cast<void *>(con_handle);
#if defined(PICO_BOARD) || defined(MOCK_PICO_PI)
        LOG_DEBUG("notifyRawPacket(0x%x) request to send notification\n", con_handle);
        att_server_request_to_send_notification(&notify_context_callback_registration, con_handle);
#endif
    }
}

void BleConnectionTracker::writeRawPacket(const hci_con_handle_t con_handle) {
    auto packets_for_handle = raw_packet_to_write.equal_range(con_handle);
    if (packets_for_handle.first != raw_packet_to_write.end()) {
        const auto connection = connections[con_handle];
        ASSERT_DEBUG(connection.hasData());
        auto &data = packets_for_handle.first->second;
#if defined(PICO_BOARD) || defined(MOCK_PICO_PI)
        const auto ret = gatt_client_write_value_of_characteristic_without_response(
            con_handle, connection.getPacketCharacteristicValueHandle(), data.size(), data.data());
        LOG_DEBUG("writeRawPacket(0x%x) ret:%d\n", con_handle, ret);
#else
        uint8_t ret = 0;
#endif
        if (ret == ERROR_CODE_SUCCESS || ret == ERROR_CODE_UNKNOWN_CONNECTION_IDENTIFIER) {
            raw_packet_to_write.erase(packets_for_handle.first);
        }
    }
    packets_for_handle = raw_packet_to_write.equal_range(con_handle);
    if (packets_for_handle.first == raw_packet_to_write.end()) {
        auto connection = connections[con_handle];
        LOG_DEBUG("writeRawPacket(0x%x) no more data\n", con_handle);
        connection.setHasData(false);
    } else {
        write_context_callback_registration.callback = &packet_can_write_without_response_handler;
        write_context_callback_registration.context = reinterpret_cast<void *>(con_handle);
#if defined(PICO_BOARD) || defined(MOCK_PICO_PI)
        LOG_DEBUG("writeRawPacket(0x%x) request to write without response\n", con_handle);
        gatt_client_request_to_write_without_response(&write_context_callback_registration, con_handle);
#endif
    }
}

hci_con_handle_t BleConnectionTracker::getAnyDuplicateHandle() {
    std::map<Peer *, hci_con_handle_t> reversed;
    for (auto &[handle, peer]: handle_peer_map) {
        if (reversed[peer] != 0) {
            //return the earlier connection - appears to get gazumped by the more recent one
            LOG_DEBUG(("rp ls: %" PRIu64 ", h ls: %" PRIu64 " \n"), connections[reversed[peer]].getTimestamp(),
                      connections[handle].getTimestamp());
            if (connections[reversed[peer]].getTimestamp() < connections[handle].getTimestamp()) {
                return reversed[peer];
            } //else
            return handle;
        }
        reversed[peer] = handle;
    }
    return 0;
}
