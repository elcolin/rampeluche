#include "LidarController.hpp"
#include "RplidarProtocol.hpp"

namespace {
constexpr uint8_t kLidarMotorPwmDuty = 100;
// Le moteur du Lidar doit atteindre sa vitesse nominale avant l'envoi de la
// commande Express Scan, faute de quoi le RPLIDAR ignore la requete.
constexpr unsigned long kMotorSpinUpDelayMs = 500;
constexpr size_t kExpressScanPayloadSize = 5;
}

void LidarController::setup()
{
    pinMode(lidar_mot_ctl_pin, OUTPUT);
    Serial2.begin(115200, SERIAL_8N1, lidar_tx_pin, lidar_rx_pin);
}

void LidarController::startExpressScan()
{
    analogWrite(lidar_mot_ctl_pin, kLidarMotorPwmDuty);
    delay(kMotorSpinUpDelayMs);
    while (!Serial2.availableForWrite());

    uint8_t payload[kExpressScanPayloadSize] = {0};
    uint8_t request[Rplidar::requestPacketSize(kExpressScanPayloadSize)];
    size_t len = Rplidar::buildRequestPacket(
        request, static_cast<uint8_t>(RplidarCmd::ExpressScan), payload, sizeof(payload));
    Serial2.write(request, len);
}

void LidarController::poll(Rplidar::StreamDecoder::PointCallback onPoints, void *userData)
{
    int available = Serial2.available();
    if (available <= 0) {
        return;
    }

    uint8_t buf[256];
    size_t toRead = static_cast<size_t>(available) > sizeof(buf) ? sizeof(buf) : static_cast<size_t>(available);
    size_t n = Serial2.readBytes(buf, toRead);
    decoder.feed(buf, n, onPoints, userData);
}
