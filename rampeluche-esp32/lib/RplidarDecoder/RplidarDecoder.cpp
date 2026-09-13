#include "RplidarDecoder.hpp"

#include <cstring>

namespace Rplidar {

namespace {

constexpr uint8_t kSyncNibble1 = 0xA;
constexpr uint8_t kSyncNibble2 = 0x5;

float normalizeDeg(float angle)
{
    while (angle < 0.0f) angle += 360.0f;
    while (angle >= 360.0f) angle -= 360.0f;
    return angle;
}

} // namespace

bool hasValidSync(const uint8_t *packet)
{
    return (packet[0] >> 4) == kSyncNibble1 && (packet[1] >> 4) == kSyncNibble2;
}

bool hasValidChecksum(const uint8_t *packet)
{
    // Checksum recu : bits[3:0] pris sur l'octet 0, bits[7:4] sur l'octet 1
    // (les nibbles hautes de ces 2 octets sont les demi-octets de synchro).
    uint8_t received = (packet[0] & 0x0F) | ((packet[1] & 0x0F) << 4);

    uint8_t computed = 0;
    for (size_t i = 2; i < kExpressPacketSize; i++) {
        computed ^= packet[i];
    }
    return computed == received;
}

float startAngleDeg(const uint8_t *packet)
{
    uint16_t raw = packet[2] | (static_cast<uint16_t>(packet[3]) << 8);
    uint16_t angle_q6 = raw & 0x7FFF;
    return angle_q6 / 64.0f;
}

bool isNewScan(const uint8_t *packet)
{
    return (packet[3] & 0x80) != 0;
}

void decodeCapsulePair(const uint8_t *prevPacket, const uint8_t *curPacket, ScanPoint out[kPointsPerPacket])
{
    float prevStart = startAngleDeg(prevPacket);
    float curStart = startAngleDeg(curPacket);

    float diffAngle = curStart - prevStart;
    if (diffAngle < 0.0f) {
        diffAngle += 360.0f; // le tour a boucle entre les 2 paquets
    }
    float angleInc = diffAngle / static_cast<float>(kPointsPerPacket);

    float currentAngle = prevStart;
    for (size_t cabin = 0; cabin < kCabinsPerPacket; cabin++) {
        const uint8_t *c = prevPacket + 4 + cabin * 5;
        uint16_t distanceAngle1 = c[0] | (static_cast<uint16_t>(c[1]) << 8);
        uint16_t distanceAngle2 = c[2] | (static_cast<uint16_t>(c[3]) << 8);
        uint8_t offsetAnglesQ3 = c[4];

        // Offset d'angle (Q3, non signe, 6 bits) : bits bas depuis
        // offset_angles_q3, bits hauts depuis les 2 bits bas du champ
        // distance_angle correspondant.
        uint8_t angleOffset1Q3 = (offsetAnglesQ3 & 0x0F) | ((distanceAngle1 & 0x03) << 4);
        uint8_t angleOffset2Q3 = (offsetAnglesQ3 >> 4) | ((distanceAngle2 & 0x03) << 4);

        // Le masquage des 2 bits bas (utilises pour l'offset d'angle) puis
        // la division par 4 pour passer de Q2 a des mm equivaut a un simple
        // decalage de 2 bits.
        float distance1 = (distanceAngle1 >> 2) / 4.0f;
        float distance2 = (distanceAngle2 >> 2) / 4.0f;

        float angle1 = currentAngle - angleOffset1Q3 / 8.0f;
        currentAngle += angleInc;
        float angle2 = currentAngle - angleOffset2Q3 / 8.0f;
        currentAngle += angleInc;

        out[cabin * kPointsPerCabin] = ScanPoint{normalizeDeg(angle1), distance1};
        out[cabin * kPointsPerCabin + 1] = ScanPoint{normalizeDeg(angle2), distance2};
    }
}

void StreamDecoder::reset()
{
    _bufPos = 0;
    _hasPrevPacket = false;
}

StreamDecoder::ByteResult StreamDecoder::pushByte(uint8_t b)
{
    if (_bufPos == 0) {
        if ((b >> 4) != kSyncNibble1) {
            // Un octet hors-synchro ici signale un trou dans le flux (perte,
            // bruit...) : on ne peut plus garantir que le prochain paquet
            // valide sera l'immediat successeur de _prevPacket, donc on
            // oublie ce dernier plutot que de risquer d'interpoler entre 2
            // paquets non consecutifs (angles/distances silencieusement
            // faux).
            _hasPrevPacket = false;
            return ByteResult::Incomplete; // en attente du 1er demi-octet de synchro
        }
    } else if (_bufPos == 1) {
        if ((b >> 4) != kSyncNibble2) {
            _hasPrevPacket = false; // idem : synchro perdue
            if ((b >> 4) == kSyncNibble1) {
                // Cet octet peut lui-meme etre le sync1 d'une nouvelle
                // trame : on le retente immediatement plutot que de perdre
                // un octet de plus a relancer la recherche depuis zero.
                _buf[0] = b;
                _bufPos = 1;
            } else {
                _bufPos = 0;
            }
            return ByteResult::Incomplete;
        }
    }

    _buf[_bufPos++] = b;
    if (_bufPos < kExpressPacketSize) {
        return ByteResult::Incomplete;
    }

    _bufPos = 0;
    return hasValidChecksum(_buf) ? ByteResult::PacketReady : ByteResult::ChecksumError;
}

size_t StreamDecoder::feed(const uint8_t *data, size_t len, PointCallback onPoints, void *user_data)
{
    size_t checksumErrors = 0;

    for (size_t i = 0; i < len; i++) {
        ByteResult result = pushByte(data[i]);

        if (result == ByteResult::Incomplete) {
            continue;
        }
        if (result == ByteResult::ChecksumError) {
            checksumErrors++;
            // Flux corrompu : on perd la reference d'angle du paquet
            // precedent plutot que d'interpoler a partir de donnees fausses.
            _hasPrevPacket = false;
            continue;
        }

        if (isNewScan(_buf)) {
            _hasPrevPacket = false;
        }

        if (_hasPrevPacket) {
            ScanPoint points[kPointsPerPacket];
            decodeCapsulePair(_prevPacket, _buf, points);
            if (onPoints) {
                onPoints(points, kPointsPerPacket, user_data);
            }
        }

        memcpy(_prevPacket, _buf, kExpressPacketSize);
        _hasPrevPacket = true;
    }

    return checksumErrors;
}

} // namespace Rplidar
