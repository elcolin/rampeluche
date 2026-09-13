#include <cmath>
#include <cstring>
#include <unity.h>
#include "RplidarDecoder.hpp"

using namespace Rplidar;

void setUp() {}
void tearDown() {}

// Construit un paquet capsule (84 octets) valide a partir de valeurs
// "humaines" : angle de depart en degres, distances en mm et offsets
// d'angle en Q3 (0..63, cf. RplidarDecoder.hpp) pour chacun des 32 points.
// Sert de reference independante de l'implementation testee : le paquet est
// assemble bit a bit selon le format documente, puis relu par le decodeur.
static void buildPacket(uint8_t *out, float startAngle, bool newScan,
                         const float distances_mm[kPointsPerPacket],
                         const uint8_t offsetsQ3[kPointsPerPacket])
{
    memset(out, 0, kExpressPacketSize);

    uint16_t angle_q6 = static_cast<uint16_t>(startAngle * 64.0f) & 0x7FFF;
    if (newScan) angle_q6 |= 0x8000;
    out[2] = angle_q6 & 0xFF;
    out[3] = (angle_q6 >> 8) & 0xFF;

    for (size_t cabin = 0; cabin < kCabinsPerPacket; cabin++) {
        uint16_t dist1_q2 = static_cast<uint16_t>(distances_mm[cabin * 2] * 4.0f);
        uint16_t dist2_q2 = static_cast<uint16_t>(distances_mm[cabin * 2 + 1] * 4.0f);
        uint8_t off1 = offsetsQ3[cabin * 2];
        uint8_t off2 = offsetsQ3[cabin * 2 + 1];

        uint16_t distanceAngle1 = (dist1_q2 << 2) | ((off1 >> 4) & 0x03);
        uint16_t distanceAngle2 = (dist2_q2 << 2) | ((off2 >> 4) & 0x03);
        uint8_t offsetAnglesQ3 = (off1 & 0x0F) | ((off2 & 0x0F) << 4);

        uint8_t *c = out + 4 + cabin * 5;
        c[0] = distanceAngle1 & 0xFF;
        c[1] = (distanceAngle1 >> 8) & 0xFF;
        c[2] = distanceAngle2 & 0xFF;
        c[3] = (distanceAngle2 >> 8) & 0xFF;
        c[4] = offsetAnglesQ3;
    }

    uint8_t checksum = 0;
    for (size_t i = 2; i < kExpressPacketSize; i++) {
        checksum ^= out[i];
    }
    out[0] = 0xA0 | (checksum & 0x0F);
    out[1] = 0x50 | ((checksum >> 4) & 0x0F);
}

static void buildUniformPacket(uint8_t *out, float startAngle, bool newScan, float distance_mm)
{
    float distances[kPointsPerPacket];
    uint8_t offsets[kPointsPerPacket] = {0};
    for (size_t i = 0; i < kPointsPerPacket; i++) distances[i] = distance_mm;
    buildPacket(out, startAngle, newScan, distances, offsets);
}

void test_valid_sync_nibbles_are_recognized()
{
    uint8_t packet[kExpressPacketSize];
    buildUniformPacket(packet, 0.0f, false, 1000.0f);
    TEST_ASSERT_TRUE(hasValidSync(packet));
}

void test_wrong_sync_nibbles_are_rejected()
{
    uint8_t packet[kExpressPacketSize];
    buildUniformPacket(packet, 0.0f, false, 1000.0f);
    packet[1] = 0x00; // casse le 2e demi-octet de synchro (attendu 0x5_)
    TEST_ASSERT_FALSE(hasValidSync(packet));
}

void test_valid_checksum_is_accepted()
{
    uint8_t packet[kExpressPacketSize];
    buildUniformPacket(packet, 42.0f, false, 1234.0f);
    TEST_ASSERT_TRUE(hasValidChecksum(packet));
}

void test_corrupted_byte_fails_checksum()
{
    uint8_t packet[kExpressPacketSize];
    buildUniformPacket(packet, 42.0f, false, 1234.0f);
    packet[10] ^= 0xFF; // corrompt un octet de donnees
    TEST_ASSERT_FALSE(hasValidChecksum(packet));
}

void test_start_angle_is_decoded_in_degrees()
{
    uint8_t packet[kExpressPacketSize];
    buildUniformPacket(packet, 123.5f, false, 500.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.02f, 123.5f, startAngleDeg(packet));
}

void test_new_scan_flag_is_decoded()
{
    uint8_t packet[kExpressPacketSize];
    buildUniformPacket(packet, 0.0f, true, 500.0f);
    TEST_ASSERT_TRUE(isNewScan(packet));

    buildUniformPacket(packet, 0.0f, false, 500.0f);
    TEST_ASSERT_FALSE(isNewScan(packet));
}

void test_decode_capsule_pair_interpolates_angles_and_distances()
{
    // 16 degres repartis sur les 32 points -> increment de 0.5 deg/point.
    uint8_t prev[kExpressPacketSize];
    uint8_t cur[kExpressPacketSize];
    buildUniformPacket(prev, 10.0f, false, 1000.0f);
    buildUniformPacket(cur, 26.0f, false, 1000.0f);

    ScanPoint points[kPointsPerPacket];
    decodeCapsulePair(prev, cur, points);

    for (size_t i = 0; i < kPointsPerPacket; i++) {
        float expectedAngle = 10.0f + i * 0.5f;
        TEST_ASSERT_FLOAT_WITHIN(0.05f, expectedAngle, points[i].angle_deg);
        TEST_ASSERT_FLOAT_WITHIN(0.5f, 1000.0f, points[i].distance_mm);
    }
}

void test_decode_capsule_pair_applies_angle_offset_correction()
{
    // Offset de 2.0 degres (Q3 = 16) soustrait a l'angle interpole du 1er point.
    float distances[kPointsPerPacket];
    uint8_t offsets[kPointsPerPacket] = {0};
    for (size_t i = 0; i < kPointsPerPacket; i++) distances[i] = 800.0f;
    offsets[0] = 16; // 16/8 = 2.0 deg

    uint8_t prev[kExpressPacketSize];
    uint8_t cur[kExpressPacketSize];
    buildPacket(prev, 0.0f, false, distances, offsets);
    buildPacket(cur, 16.0f, false, distances, offsets);

    ScanPoint points[kPointsPerPacket];
    decodeCapsulePair(prev, cur, points);

    // Point 0 : angle interpole = 0.0, moins l'offset de 2.0 deg -> -2.0 -> 358.0
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 358.0f, points[0].angle_deg);
}

void test_decode_capsule_pair_handles_wraparound_across_zero_degrees()
{
    uint8_t prev[kExpressPacketSize];
    uint8_t cur[kExpressPacketSize];
    buildUniformPacket(prev, 350.0f, false, 500.0f);
    buildUniformPacket(cur, 2.0f, false, 500.0f); // a boucle : diff reel = 12 deg

    ScanPoint points[kPointsPerPacket];
    decodeCapsulePair(prev, cur, points);

    TEST_ASSERT_FLOAT_WITHIN(0.05f, 350.0f, points[0].angle_deg);
    // Dernier point avant `cur` : 350 + 31 * (12/32) ~= 361.625 -> 1.625
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 1.625f, points[kPointsPerPacket - 1].angle_deg);
}

namespace {
struct CapturedPoints {
    ScanPoint points[kPointsPerPacket];
    size_t count = 0;
    int callCount = 0;
};

void capturePoints(const ScanPoint *points, size_t count, void *user_data)
{
    auto *captured = static_cast<CapturedPoints *>(user_data);
    captured->count = count;
    memcpy(captured->points, points, count * sizeof(ScanPoint));
    captured->callCount++;
}
} // namespace

void test_stream_decoder_resyncs_after_leading_garbage()
{
    uint8_t packet1[kExpressPacketSize];
    uint8_t packet2[kExpressPacketSize];
    buildUniformPacket(packet1, 0.0f, true, 1000.0f);
    buildUniformPacket(packet2, 10.0f, false, 1000.0f);

    uint8_t stream[5 + kExpressPacketSize * 2];
    uint8_t garbage[5] = {0x00, 0xFF, 0x12, 0x34, 0x56};
    memcpy(stream, garbage, sizeof(garbage));
    memcpy(stream + sizeof(garbage), packet1, kExpressPacketSize);
    memcpy(stream + sizeof(garbage) + kExpressPacketSize, packet2, kExpressPacketSize);

    StreamDecoder decoder;
    CapturedPoints captured;
    size_t errors = decoder.feed(stream, sizeof(stream), capturePoints, &captured);

    TEST_ASSERT_EQUAL_UINT32(0, errors);
    TEST_ASSERT_EQUAL_INT(1, captured.callCount); // 1 seule paire complete (packet1 + packet2)
    TEST_ASSERT_EQUAL_UINT32(kPointsPerPacket, captured.count);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 0.0f, captured.points[0].angle_deg);
}

void test_stream_decoder_does_not_pair_packets_across_a_sync_loss()
{
    // Si un octet parasite casse l'alignement entre 2 paquets par ailleurs
    // valides, le decodeur ne doit pas apparier le paquet mis en cache
    // *avant* le trou avec celui trouve *apres* : rien ne garantit alors
    // qu'ils sont angulairement consecutifs (cf. finding code-review sur la
    // PR : un paquet perdu au milieu du flux pouvait sinon produire un lot
    // de points silencieusement faux, interpole sur un ecart angulaire
    // n'ayant jamais existe).
    uint8_t packet1[kExpressPacketSize];
    uint8_t packet2[kExpressPacketSize];
    uint8_t packet3[kExpressPacketSize];
    buildUniformPacket(packet1, 0.0f, true, 1000.0f);
    buildUniformPacket(packet2, 100.0f, false, 1000.0f);
    buildUniformPacket(packet3, 110.0f, false, 1000.0f);

    uint8_t stream[kExpressPacketSize * 3 + 1];
    memcpy(stream, packet1, kExpressPacketSize);
    stream[kExpressPacketSize] = 0x00; // casse la synchro entre packet1 et packet2
    memcpy(stream + kExpressPacketSize + 1, packet2, kExpressPacketSize);
    memcpy(stream + kExpressPacketSize + 1 + kExpressPacketSize, packet3, kExpressPacketSize);

    StreamDecoder decoder;
    CapturedPoints captured;
    decoder.feed(stream, sizeof(stream), capturePoints, &captured);

    // Une seule paire decodee (packet2+packet3), jamais (packet1+packet2).
    TEST_ASSERT_EQUAL_INT(1, captured.callCount);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 100.0f, captured.points[0].angle_deg);
}

void test_stream_decoder_reports_checksum_errors()
{
    uint8_t packet[kExpressPacketSize];
    buildUniformPacket(packet, 0.0f, true, 1000.0f);
    packet[20] ^= 0xFF; // corrompt le paquet apres coup

    StreamDecoder decoder;
    CapturedPoints captured;
    size_t errors = decoder.feed(packet, sizeof(packet), capturePoints, &captured);

    TEST_ASSERT_EQUAL_UINT32(1, errors);
    TEST_ASSERT_EQUAL_INT(0, captured.callCount); // rien a publier sans paire valide
}

void test_stream_decoder_needs_two_packets_before_first_callback()
{
    // newScan=false sur les 2 paquets : on reste au milieu d'un tour, donc le
    // decodeur ne doit pas jeter le paquet precedent (cf. test dedie a
    // isNewScan ci-dessus pour ce cas la).
    uint8_t packet[kExpressPacketSize];
    buildUniformPacket(packet, 0.0f, false, 1000.0f);

    StreamDecoder decoder;
    CapturedPoints captured;
    decoder.feed(packet, sizeof(packet), capturePoints, &captured);
    TEST_ASSERT_EQUAL_INT(0, captured.callCount); // 1er paquet: juste mis en cache

    decoder.feed(packet, sizeof(packet), capturePoints, &captured);
    TEST_ASSERT_EQUAL_INT(1, captured.callCount); // 2e paquet: la paire peut etre decodee
}

int main(int argc, char **argv)
{
    UNITY_BEGIN();
    RUN_TEST(test_valid_sync_nibbles_are_recognized);
    RUN_TEST(test_wrong_sync_nibbles_are_rejected);
    RUN_TEST(test_valid_checksum_is_accepted);
    RUN_TEST(test_corrupted_byte_fails_checksum);
    RUN_TEST(test_start_angle_is_decoded_in_degrees);
    RUN_TEST(test_new_scan_flag_is_decoded);
    RUN_TEST(test_decode_capsule_pair_interpolates_angles_and_distances);
    RUN_TEST(test_decode_capsule_pair_applies_angle_offset_correction);
    RUN_TEST(test_decode_capsule_pair_handles_wraparound_across_zero_degrees);
    RUN_TEST(test_stream_decoder_resyncs_after_leading_garbage);
    RUN_TEST(test_stream_decoder_does_not_pair_packets_across_a_sync_loss);
    RUN_TEST(test_stream_decoder_reports_checksum_errors);
    RUN_TEST(test_stream_decoder_needs_two_packets_before_first_callback);
    return UNITY_END();
}
