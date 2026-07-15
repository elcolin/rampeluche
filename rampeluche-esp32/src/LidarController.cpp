#include "LidarController.hpp"

void LidarController::setup()
{
    pinMode(lidar_mot_ctl_pin, OUTPUT);
    Serial2.begin(115200, SERIAL_8N1, lidar_tx_pin, lidar_rx_pin);

}

// struct requestPacket{
//     uint8_t start_flag;
//     uint8_t command;
//     uint8_t payload_size;
//     uint8_t payload_data[255];
//     uint8_t checksum;
// };

// struct responsePacket{
//     uint8_t start_flag1;
//     uint8_t start_flag2;
//     uint32_t data_length;
//     uint8_t mode;
//     uint8_t data_type;
// };


uint8_t LidarController::compute_checksum(uint8_t cmd_type, const uint8_t *payload, size_t payload_len) {
    uint8_t checksum = 0;
    checksum ^= 0xA5;
    checksum ^= cmd_type;
    if (payload_len > 0) {
        checksum ^= (uint8_t)payload_len;
        for (size_t i = 0; i < payload_len; i++) {
            checksum ^= payload[i];
        }
    }
    return checksum;
}

size_t LidarController::build_request(uint8_t *out, uint8_t cmd_type, const uint8_t *payload, size_t payload_len) {
    size_t idx = 0;
    out[idx++] = 0xA5;
    out[idx++] = cmd_type;
    out[idx++] = (uint8_t)payload_len;
    if (payload_len < 0)
        throw std::length_error("Wrong payload size");
    for (size_t i = 0; i < payload_len; i++) {
        out[idx++] = payload[i];
    }
    out[idx++] = compute_checksum(cmd_type, payload, payload_len);
    return idx;
}

void LidarController::startExpressScan()
{
    analogWrite(lidar_mot_ctl_pin, 100);
    uint8_t payload[64] = {0};
    uint8_t out[64] = {0};
    delay(500);
    while (!Serial2.availableForWrite());

    // build_request(out, RplidarCmd::ExpressScan, payload, 0);
    int len = build_request(out, static_cast<uint8_t>(RplidarCmd::ExpressScan), payload, 5);
    // Serial2.write(0xA5);
    // Serial2.write(0x82);
//     uint8_t express_scan_cmd[] = {
//   0xA5, 0x82,
//   0x05, 0x00, 0x00, 0x00, 0x00, 0x00,
//   0x22
// };
    for (int i = 0; i < len; i++)
    {
        Serial.printf("%X\n", out[i]);
    }
    Serial2.write(out, len);

}