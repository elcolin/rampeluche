#pragma once
#include <Arduino.h>

#include "pins.hpp"
#include "RplidarDecoder.hpp"

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
        Rplidar::StreamDecoder decoder;
    public:
        // void generate_request_packet(uint8_t command, uint8_t *data, uint8_t payload_size);
        LidarController() = default;
        ~LidarController() = default;
        void startExpressScan();
        void getSampleRate();
        void setup();

        // Lit les octets disponibles sur l'UART du lidar (Serial2) et decode
        // les paquets Express Scan recus. Appelle onPoints(points, count)
        // avec les points fraichement decodes (par lots de
        // Rplidar::kPointsPerPacket = 32) des qu'une paire de paquets
        // consecutifs a pu etre validee. A appeler regulierement depuis
        // loop() ; ne bloque pas s'il n'y a pas de donnees disponibles.
        void poll(Rplidar::StreamDecoder::PointCallback onPoints, void *user_data = nullptr);
};
