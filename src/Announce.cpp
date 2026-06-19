#include "Announce.h"

#include <utility>

#if defined(PICO_BOARD) || defined(MOCK_PICO_PI)
#include "Debugging.h"
#endif

Announce::Announce()
    : PacketBase(type_announce) {
}

Announce::Announce(const uint8_t type, const uint8_t version, BinaryReader &reader)
    : PacketBase(type, version, reader) {

    const auto name_len = reader.read_uint8();
    const auto name_data = reader.read_data(name_len);
    if (name_data) {
        name = std::string(reinterpret_cast<const char *>(name_data), name_len);
        LOG_DEBUG("name_len: %d, name:%s\n", name_len, name.c_str());
    } else {
        name = "";
    }
}

void Announce::setName(std::string value) {
    name = std::move(value);
}

const std::string &Announce::getName() const {
    return name;
}

void Announce::writePacket(std::vector<uint8_t> &vector) {
    PacketBase::writePacket(vector);
}

void Announce::writePacketPayload(BinaryWriter &writer) {
    PacketBase::writePacketPayload(writer);

    const auto name_len = static_cast<uint8_t>(std::min(static_cast<size_t>(255), name.size()));
    writer.write_uint8(name_len);
    writer.write_data(name, name_len);
}
