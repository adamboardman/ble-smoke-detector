#ifndef ANNOUNCE_H
#define ANNOUNCE_H

#include "Base.h"

class Announce final : public Base {
public:
    Announce();

    Announce(uint8_t version, BinaryReader &reader);

    void setName(const std::string &value);

    void setNoisePublicKey(const std::string &value);

    void setSigningPublicKey(const std::string &value);

    [[nodiscard]] const std::string &getName() const;

    [[nodiscard]] const std::string &getSigningPublicKey() const;

    [[nodiscard]] uint32_t getPayloadLength() override;

    [[nodiscard]] std::size_t getAnnounceHash() const;

    void writePacketPayload(BinaryWriter &writer) override;

    void setLatitudeI(int32_t value);

    void setLongitudeI(int32_t value);

    void setAltitude(int32_t value);

private:
    std::string name{};
    std::string noise_public_key{};
    std::string signing_public_key{};
    std::vector<uint64_t> direct_neighbors{};
    int32_t latitude_i = 0;
    int32_t longitude_i = 0;
    int32_t altitude = 0;
};

#endif