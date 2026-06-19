#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "include/ble_types.h"

class Peer {
public:
    Peer();

    Peer(uint64_t id, std::string peer_name);

    bool operator==(const Peer &other) const;

    void updateName(const std::string &peer_name);

    [[nodiscard]] uint64_t getId() const;

    std::string &getName();

    void setId(uint64_t new_id);

    void updateNoisePublicKey(const std::vector<uint8_t> &new_public_key);

    std::vector<uint8_t> &getPublicKey();

    [[nodiscard]] uint8_t getAnnounceTtl() const;

    void setAnnounceTtl(uint8_t ttl);

    void setConnectionHandle(uint16_t hci_con_handle);

private:
    uint64_t id{};
    std::string name{};
    std::vector<uint8_t> public_key{};
    uint8_t max_ttl{};
    uint16_t connection_handle{};
};

