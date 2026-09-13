#include <cstdint>
#include <cstdio>
#include <vector>
#include <unity.h>
#include "RplidarDecoder.hpp"

using namespace Rplidar;

// Capture reelle (facultative) du flux Express Scan du RPLIDAR A2M8. Non
// versionnee (donnees binaires specifiques a un capteur/une session) : voir
// test/fixtures/README.md pour savoir comment la generer avec le materiel.
static const char *kFixturePath = "test/fixtures/rplidar_a2m8_express_scan.bin";

void setUp() {}
void tearDown() {}

static bool readFixture(std::vector<uint8_t> &out)
{
    FILE *f = fopen(kFixturePath, "rb");
    if (!f) {
        return false;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    out.resize(size > 0 ? static_cast<size_t>(size) : 0);
    if (!out.empty()) {
        size_t n = fread(out.data(), 1, out.size(), f);
        out.resize(n);
    }
    fclose(f);
    return true;
}

namespace {

struct Stats {
    size_t packetsDecoded = 0;
    size_t pointsDecoded = 0;
    size_t pointsWithEcho = 0;
    float minDistance = 1e9f;
    float maxDistance = 0.0f;
    bool anglesSeenPerBucket[36] = {false}; // secteurs de 10 degres
};

void collect(const ScanPoint *points, size_t count, void *user_data)
{
    auto *stats = static_cast<Stats *>(user_data);
    stats->packetsDecoded++;
    for (size_t i = 0; i < count; i++) {
        stats->pointsDecoded++;
        if (points[i].distance_mm > 0.0f) {
            stats->pointsWithEcho++;
            if (points[i].distance_mm < stats->minDistance) stats->minDistance = points[i].distance_mm;
            if (points[i].distance_mm > stats->maxDistance) stats->maxDistance = points[i].distance_mm;
        }
        int bucket = static_cast<int>(points[i].angle_deg / 10.0f);
        if (bucket >= 0 && bucket < 36) stats->anglesSeenPerBucket[bucket] = true;
    }
}

} // namespace

void test_real_capture_decodes_with_low_error_rate_and_plausible_values()
{
    std::vector<uint8_t> data;
    if (!readFixture(data) || data.empty()) {
        TEST_IGNORE_MESSAGE("Pas de capture reelle : voir test/fixtures/README.md pour en generer une.");
        return;
    }

    StreamDecoder decoder;
    Stats stats;
    size_t checksumErrors = decoder.feed(data.data(), data.size(), collect, &stats);

    TEST_ASSERT_TRUE_MESSAGE(stats.packetsDecoded > 0, "aucun paquet decode dans la capture");

    // Un flux reel contient forcement quelques octets de bruit/desalignement
    // au demarrage, mais l'essentiel des paquets doit passer le checksum.
    float errorRate = static_cast<float>(checksumErrors) /
        static_cast<float>(checksumErrors + stats.packetsDecoded);
    TEST_ASSERT_TRUE_MESSAGE(errorRate < 0.05f, "trop de paquets rejetes par le checksum (>5%)");

    // Le A2M8 mesure de ~0.15m a ~12m ; on verifie juste que les distances
    // restent dans une plage physiquement plausible (pas de garbage decode).
    TEST_ASSERT_TRUE_MESSAGE(stats.pointsWithEcho > 0, "aucun point avec echo valide");
    TEST_ASSERT_TRUE_MESSAGE(stats.minDistance >= 50.0f, "distance minimale invraisemblable (<5cm)");
    TEST_ASSERT_TRUE_MESSAGE(stats.maxDistance < 12000.0f, "distance maximale invraisemblable (>12m)");

    // Verifie qu'on decode bien des angles varies (et pas une valeur figee) ;
    // seuil bas pour rester robuste a une capture courte ou un moteur lent.
    int bucketsSeen = 0;
    for (bool seen : stats.anglesSeenPerBucket) {
        if (seen) bucketsSeen++;
    }
    TEST_ASSERT_TRUE_MESSAGE(bucketsSeen >= 5, "angles decodes trop peu varies");
}

int main(int argc, char **argv)
{
    UNITY_BEGIN();
    RUN_TEST(test_real_capture_decodes_with_low_error_rate_and_plausible_values);
    return UNITY_END();
}
