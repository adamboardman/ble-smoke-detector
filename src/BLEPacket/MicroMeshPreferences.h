#ifndef MICRO_MESH_PREFERENCES_H
#define MICRO_MESH_PREFERENCES_H

#include "Base.h"

class MicroMeshPreferences final : public Base {
public:
    MicroMeshPreferences();

    MicroMeshPreferences(uint8_t version, BinaryReader &reader);

    bool operator==(const MicroMeshPreferences &other) const;

    [[nodiscard]] uint32_t getPayloadLength() override;

    void writePacketPayload(BinaryWriter &writer) override;

    void setKey(const char *the_key);

    void setSsid(const char *an_ssid);

    void setPassword(const char *the_password);

    void setLatitudeI(int lat);

    void setLongitudeI(int lon);

    void setAltitude(int alt);

    [[nodiscard]] const std::string &getKey() const;

    [[nodiscard]] const std::string &getSsid() const;

    [[nodiscard]] const std::string &getPassword() const;

    [[nodiscard]] int32_t getLatitudeI() const;

    [[nodiscard]] int32_t getLongitudeI() const;

    [[nodiscard]] int32_t getAltitude() const;


private:
    std::string key{};
    std::string ssid{};
    std::string password{};
    int32_t latitude_i = 0;
    int32_t longitude_i = 0;
    int32_t altitude = 0;
};

#endif
