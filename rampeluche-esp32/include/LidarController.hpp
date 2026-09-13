#pragma once
#include <Arduino.h>

#include "pins.hpp"
#include "RplidarDecoder.hpp"

// Commandes du protocole RPLIDAR (cf.
// datasheet/lidar/series_protocol_LR001_SLAMTEC_rplidar_series_protocol).
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

// Pilote le RPLIDAR sur Serial2 : demarre l'Express Scan et fait transiter
// les octets recus vers Rplidar::StreamDecoder (lib/RplidarDecoder) pour
// decodage. poll() est a appeler regulierement depuis loop() ; elle ne
// bloque pas s'il n'y a pas de donnees disponibles sur l'UART.
class LidarController {
    public:
        LidarController() = default;
        ~LidarController() = default;

        void setup();
        void startExpressScan();
        void poll(Rplidar::StreamDecoder::PointCallback onPoints, void *userData = nullptr);

    private:
        Rplidar::StreamDecoder decoder;
};
