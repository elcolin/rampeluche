#pragma once

#include <cstddef>
#include <cstdint>

// Decodage des paquets "Express Scan" (capsule classique) renvoyes par le
// RPLIDAR A2M8 en reponse a la commande RplidarCmd::ExpressScan (0x82).
// Logique pure, sans dependance materielle : testable hors ESP32
// (voir test/test_rplidar_decoder). L'algorithme reproduit celui du SDK
// officiel SLAMTEC (handler_capsules.cpp, UnpackerHandler_CapsuleNode).
//
// Format d'un paquet capsule (84 octets) :
//   octet0    : sync1 (bits[7:4]=0xA) | checksum bits[3:0] (bits[3:0])
//   octet1    : sync2 (bits[7:4]=0x5) | checksum bits[7:4] (bits[3:0])
//   octets2-3 : start_angle_sync_q6 (u16 little endian)
//               bit15=debut d'un nouveau tour, bits[14:0]=angle de depart
//               du paquet, en Q6 (degres * 64)
//   octets4-83: 16 "cabines" de 5 octets, chacune decrivant 2 points :
//     - distance_angle_1 (u16 LE) : bits[15:2]=distance en Q2 (mm*4),
//                                   bits[1:0]=bits hauts[5:4] de l'offset
//                                   d'angle du point 1
//     - distance_angle_2 (u16 LE) : idem pour le point 2
//     - offset_angles_q3 (u8)     : bits[3:0]=bits bas[3:0] de l'offset du
//                                   point 1 (Q3, 1/8 de degre)
//                                   bits[7:4]=bits bas[3:0] de l'offset du
//                                   point 2
//
// Chaque paquet ne porte un angle fiable qu'a son origine (start_angle) ;
// l'angle de chacun des 32 points est obtenu en interpolant lineairement
// entre les angles de depart de deux paquets consecutifs, puis corrige par
// le petit offset (Q3) porte par sa cabine. D'ou l'API a deux paquets de
// decodeCapsulePair() / le buffering interne de StreamDecoder.
namespace Rplidar {

constexpr size_t kExpressPacketSize = 84;
constexpr size_t kCabinsPerPacket = 16;
constexpr size_t kPointsPerCabin = 2;
constexpr size_t kPointsPerPacket = kCabinsPerPacket * kPointsPerCabin;

struct ScanPoint {
    float angle_deg;    // 0..360
    float distance_mm;  // 0 si pas d'echo valide
};

// Verifie les 2 demi-octets de synchronisation (0xA, 0x5) en tete du paquet.
bool hasValidSync(const uint8_t *packet);

// Recalcule le checksum (XOR octet a octet des octets 2..83, cf. protocole
// SLAMTEC) et le compare a celui transporte dans les 2 premiers octets.
bool hasValidChecksum(const uint8_t *packet);

// Angle de depart du paquet, en degres (0..360).
float startAngleDeg(const uint8_t *packet);

// Indique si ce paquet marque le debut d'un nouveau tour complet.
bool isNewScan(const uint8_t *packet);

// Decode une paire de paquets consecutifs (deja verifies par l'appelant :
// hasValidSync + hasValidChecksum) en kPointsPerPacket points de mesure,
// dates entre le debut de `prevPacket` et le debut de `curPacket`.
void decodeCapsulePair(const uint8_t *prevPacket, const uint8_t *curPacket, ScanPoint out[kPointsPerPacket]);

// Decodeur "streaming" a nourrir avec les octets bruts recus sur l'UART du
// lidar. Se resynchronise automatiquement sur les demi-octets 0xA/0x5 en cas
// de perte d'alignement (redemarrage, octet perdu, bruit...), verifie le
// checksum de chaque paquet, et restitue les points de mesure au fur et a
// mesure via un callback.
class StreamDecoder {
public:
    using PointCallback = void (*)(const ScanPoint *points, size_t count, void *user_data);

    StreamDecoder() = default;

    // Oublie tout octet/paquet en cours de reception.
    void reset();

    // Traite `len` octets recus depuis le port serie du lidar. Appelle
    // `onPoints` (si non nul) a chaque fois qu'une paire de paquets valides
    // vient d'etre decodee (kPointsPerPacket points). Retourne le nombre de
    // paquets recus dont le checksum etait invalide pendant cet appel (utile
    // pour du diagnostic ; ne compte pas les octets de resynchronisation).
    size_t feed(const uint8_t *data, size_t len, PointCallback onPoints, void *user_data = nullptr);

private:
    enum class ByteResult {
        Incomplete,
        ChecksumError,
        PacketReady,
    };

    ByteResult pushByte(uint8_t b);

    uint8_t _buf[kExpressPacketSize] = {0};
    size_t _bufPos = 0;

    bool _hasPrevPacket = false;
    uint8_t _prevPacket[kExpressPacketSize] = {0};
};

} // namespace Rplidar
