#pragma once
#include <Arduino.h>

#include "pins.hpp"

struct lidarPacket {

};

#include <cstdint>

enum class RplidarCmd : uint8_t {
    Stop            = 0x25,
    Reset           = 0x40,
    Scan            = 0x20,
    ExpressScan     = 0x82,
    GetInfo         = 0x50,
    GetHealth       = 0x52,
    GetSampleRate   = 0x59,
    GetLidarConf    = 0x84,
};

class LidarController {
    private:
        uint8_t compute_checksum(size_t size);
        size_t build_request(uint8_t *out, uint8_t cmd_type, const uint8_t *payload, size_t payload_len);
        uint8_t compute_checksum(uint8_t cmd_type, const uint8_t *payload, size_t payload_len);
    public:
        // void generate_request_packet(uint8_t command, uint8_t *data, uint8_t payload_size);
        LidarController() = default;
        ~LidarController() = default;
        void startExpressScan();
        void setup();
};
