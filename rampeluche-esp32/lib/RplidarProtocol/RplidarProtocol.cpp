#include "RplidarProtocol.hpp"

namespace {

uint8_t computeChecksum(uint8_t cmdType, const uint8_t *payload, size_t payloadLen)
{
    uint8_t checksum = 0xA5;
    checksum ^= cmdType;
    if (payloadLen > 0) {
        checksum ^= static_cast<uint8_t>(payloadLen);
        for (size_t i = 0; i < payloadLen; i++) {
            checksum ^= payload[i];
        }
    }
    return checksum;
}

}

namespace Rplidar {

size_t buildRequestPacket(uint8_t *out, uint8_t cmdType, const uint8_t *payload, size_t payloadLen)
{
    size_t idx = 0;
    out[idx++] = 0xA5;
    out[idx++] = cmdType;
    out[idx++] = static_cast<uint8_t>(payloadLen);
    for (size_t i = 0; i < payloadLen; i++) {
        out[idx++] = payload[i];
    }
    out[idx++] = computeChecksum(cmdType, payload, payloadLen);
    return idx;
}

}
