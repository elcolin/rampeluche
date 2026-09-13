#pragma once
#include <cstddef>
#include <cstdint>

// Encodage des paquets de requete envoyes au RPLIDAR (cf.
// datasheet/lidar/series_protocol_LR001_SLAMTEC_rplidar_series_protocol).
// Logique pure, sans dependance a Arduino.h, testee sous l'environnement
// `native` (voir test/test_rplidar_protocol).
namespace Rplidar {

// Construit dans `out` un paquet de requete :
// Start Flag (0xA5) + Cmd + Payload Size + Payload + Checksum (XOR).
// `out` doit pouvoir contenir au moins 4 + payloadLen octets.
// Retourne le nombre d'octets ecrits.
size_t buildRequestPacket(uint8_t *out, uint8_t cmdType, const uint8_t *payload, size_t payloadLen);

}
