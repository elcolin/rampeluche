#pragma once
#include <cstddef>
#include <cstdint>

// Encodage des paquets de requete envoyes au RPLIDAR (cf.
// datasheet/lidar/series_protocol_LR001_SLAMTEC_rplidar_series_protocol).
// Logique pure, sans dependance a Arduino.h, testee sous l'environnement
// `native` (voir test/test_rplidar_protocol).
namespace Rplidar {

// Start Flag (1) + Cmd (1) + Payload Size (1) + Checksum (1).
constexpr size_t kRequestPacketOverheadBytes = 4;

// Taille exacte (en octets) du paquet produit par buildRequestPacket() pour
// un payload de `payloadLen` octets. A utiliser pour dimensionner le buffer
// appelant (ex. `uint8_t request[requestPacketSize(payloadLen)]`) plutot
// qu'un magic number : un buffer sous-dimensionne provoque un debordement.
constexpr size_t requestPacketSize(size_t payloadLen)
{
    return kRequestPacketOverheadBytes + payloadLen;
}

// Construit dans `out` un paquet de requete :
// Start Flag (0xA5) + Cmd + Payload Size + Payload + Checksum (XOR).
// `out` doit pouvoir contenir au moins requestPacketSize(payloadLen) octets.
// Retourne le nombre d'octets ecrits.
size_t buildRequestPacket(uint8_t *out, uint8_t cmdType, const uint8_t *payload, size_t payloadLen);

}
