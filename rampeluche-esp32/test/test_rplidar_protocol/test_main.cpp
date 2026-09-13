#include <unity.h>
#include "RplidarProtocol.hpp"

using namespace Rplidar;

void setUp() {}
void tearDown() {}

// Vecteur de reference tire de la doc protocole RPLIDAR (commande Express
// Scan, payload de 5 octets a zero) : verifie a la fois la structure du
// paquet et le calcul du checksum XOR.
void test_build_request_packet_encodes_express_scan_with_payload()
{
    const uint8_t payload[5] = {0};
    uint8_t out[16] = {0};

    size_t len = buildRequestPacket(out, 0x82, payload, sizeof(payload));

    const uint8_t expected[] = {0xA5, 0x82, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22};
    TEST_ASSERT_EQUAL_size_t(sizeof(expected), len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, out, sizeof(expected));
}

void test_build_request_packet_encodes_command_without_payload()
{
    uint8_t out[16] = {0};

    size_t len = buildRequestPacket(out, 0x50, nullptr, 0);

    const uint8_t expected[] = {0xA5, 0x50, 0x00, 0xF5};
    TEST_ASSERT_EQUAL_size_t(sizeof(expected), len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, out, sizeof(expected));
}

int main(int argc, char **argv)
{
    UNITY_BEGIN();
    RUN_TEST(test_build_request_packet_encodes_express_scan_with_payload);
    RUN_TEST(test_build_request_packet_encodes_command_without_payload);
    return UNITY_END();
}
