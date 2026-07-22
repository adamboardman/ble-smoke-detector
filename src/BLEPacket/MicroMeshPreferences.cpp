#include "MicroMeshPreferences.h"

#include "int_types.h"
#include "Debugging.h"

MicroMeshPreferences::MicroMeshPreferences()
    : Base(type_micro_mesh_preferences) {
}

MicroMeshPreferences::MicroMeshPreferences(const uint8_t version, BinaryReader &reader)
    : Base(type_micro_mesh_preferences, version, reader) {
    uint16_t remainder;
    const auto tail = reader.read_remainder_len();
    do {
        const auto data_type = reader.read_uint8();
        const auto data_length = reader.read_uint8();
        if (data_type == tlv_micro_mesh_preferences_latitude) {
            if (data_length == sizeof(int32_t)) {
                latitude_i = reader.read_int32();
                LOG_DEBUG("latitude_i: %d\n", latitude_i);
            } else {
                setMalformed(true);
            }
        } else if (data_type == tlv_micro_mesh_preferences_longitude) {
            if (data_length == sizeof(uint32_t)) {
                longitude_i = reader.read_int32();
                LOG_DEBUG("longitude_i: %d\n", longitude_i);
            } else {
                setMalformed(true);
            }
        } else if (data_type == tlv_micro_mesh_preferences_altitude) {
            if (data_length == sizeof(int32_t)) {
                altitude = reader.read_int32();
                LOG_DEBUG("altitude: %d\n", altitude);
            } else {
                setMalformed(true);
            }
        } else if (const uint8_t *data = reader.read_data(data_length)) {
            switch (data_type) {
                case tlv_micro_mesh_preferences_key:
                    key = std::string(reinterpret_cast<const char *>(data), data_length);
                    // LOG_DEBUG("key: %s\n", key.c_str());
                    break;
                case tlv_micro_mesh_preferences_ssid:
                    ssid = std::string(reinterpret_cast<const char *>(data), data_length);
                    LOG_DEBUG("ssid: %s\n", ssid.c_str());
                    break;
                case tlv_micro_mesh_preferences_password:
                    password = std::string(reinterpret_cast<const char *>(data), data_length);
                    // LOG_DEBUG("password: %s\n", password.c_str());
                    break;
                default:
                    //ignore unknown future type - data skipped and ignored
                    break;
            }
        } else {
            setMalformed(true);
        }
        remainder = reader.read_remainder_len();
    } while (!isMalformed() && remainder > tail - MicroMeshPreferences::getPayloadLength());
    handleReaderRemainder(reader);
}

bool MicroMeshPreferences::operator==(const MicroMeshPreferences &other) const {
    return key == other.getKey() && ssid == other.getSsid() && password == other.getPassword() && latitude_i == other.
           getLatitudeI() && longitude_i == other.getLongitudeI() && altitude == other.getAltitude();
}

uint32_t MicroMeshPreferences::getPayloadLength() {
    if (Base::getPayloadLength() > 0) return Base::getPayloadLength();
    uint32_t len = 0;
    if (!key.empty()) len += variableLength(tlv_micro_mesh_preferences_key, key.size());
    if (!ssid.empty()) len += variableLength(tlv_micro_mesh_preferences_ssid, ssid.size());
    if (!password.empty()) len += variableLength(tlv_micro_mesh_preferences_password, password.size());
    if (latitude_i != 0) len += variableLength(tlv_micro_mesh_preferences_latitude, sizeof(uint32_t));
    if (longitude_i != 0) len += variableLength(tlv_micro_mesh_preferences_longitude, sizeof(uint32_t));
    if (altitude != 0) len += variableLength(tlv_micro_mesh_preferences_altitude, sizeof(uint32_t));
    setPayloadLength(len);
    return len;
}

void MicroMeshPreferences::writePacketPayload(BinaryWriter &writer) {
    if (!key.empty()) writeVariable(writer, tlv_micro_mesh_preferences_key, key);
    if (!ssid.empty()) writeVariable(writer, tlv_micro_mesh_preferences_ssid, ssid);
    if (!password.empty()) writeVariable(writer, tlv_micro_mesh_preferences_password, password);
    if (latitude_i != 0) writeVariable(writer, tlv_micro_mesh_preferences_latitude, latitude_i);
    if (longitude_i != 0) writeVariable(writer, tlv_micro_mesh_preferences_longitude, longitude_i);
    if (altitude != 0) writeVariable(writer, tlv_micro_mesh_preferences_altitude, altitude);
}

void MicroMeshPreferences::setKey(const char *the_key) {
    key = the_key;
}

void MicroMeshPreferences::setSsid(const char *an_ssid) {
    ssid = an_ssid;
}

void MicroMeshPreferences::setPassword(const char *the_password) {
    password = the_password;
}

void MicroMeshPreferences::setLatitudeI(const int lat) {
    latitude_i = lat;
}

void MicroMeshPreferences::setLongitudeI(const int lon) {
    longitude_i = lon;
}

void MicroMeshPreferences::setAltitude(const int alt) {
    altitude = alt;
}

const std::string &MicroMeshPreferences::getKey() const {
    return key;
}

const std::string &MicroMeshPreferences::getSsid() const {
    return ssid;
}

const std::string &MicroMeshPreferences::getPassword() const {
    return password;
}

int32_t MicroMeshPreferences::getLatitudeI() const {
    return latitude_i;
}

int32_t MicroMeshPreferences::getLongitudeI() const {
    return longitude_i;
}

int32_t MicroMeshPreferences::getAltitude() const {
    return altitude;
}
