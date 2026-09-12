#include <Arduino.h>
#include "DriverMotor.h"
#include "pins.hpp"
#include "LidarController.hpp"
#include <WiFi.h>

const uint8_t encoderA = 15;
const uint8_t encoderB = 16;
DriverMotor MotDriver;
LidarController LidCtl;

volatile bool A;
volatile bool B;

//   bool A = digitalRead(encoderA);
//   bool B = digitalRead(encoderB);

volatile long encoderCount = 0;
volatile unsigned long lastInterruptTime = 0;

void IRAM_ATTR encoderISR() {
    A = digitalRead(encoderA);
    B = digitalRead(encoderB);
    if (A && B)
        encoderCount++;
}

WiFiServer server(1234);


void setup() {

    Serial.begin(115200);
    LidCtl.setup();
    WiFi.softAP(
        "ESP32_Control",
        "12345678"
    );

    Serial.println(WiFi.softAPIP());

    server.begin();
    MotDriver.setupDriver();
}


constexpr uint8_t FORWARD_SPEED  = 70;  // % PWM en marche avant
constexpr uint8_t BACKWARD_SPEED = 35;  // % PWM en marche arriere
constexpr uint8_t TURN_SPEED     = 35;  // % PWM en pivot sur place
constexpr unsigned long KEY_TIMEOUT_MS = 500; // coupure moteur si pas de touche recue

void input_key(char c)
{
    switch (c)
    {
        case 'w':
            MotDriver.Motors[LEFT].setMotorForward(FORWARD_SPEED);
            MotDriver.Motors[RIGHT].setMotorForward(FORWARD_SPEED);
            break;
        case 's':
            MotDriver.Motors[LEFT].setMotorBackward(BACKWARD_SPEED);
            MotDriver.Motors[RIGHT].setMotorBackward(BACKWARD_SPEED);
            break;
        case 'a': // pivot vers la gauche
            MotDriver.Motors[LEFT].setMotorBackward(TURN_SPEED);
            MotDriver.Motors[RIGHT].setMotorForward(TURN_SPEED);
            break;
        case 'd': // pivot vers la droite
            MotDriver.Motors[LEFT].setMotorForward(TURN_SPEED);
            MotDriver.Motors[RIGHT].setMotorBackward(TURN_SPEED);
            break;
        default: // touche inconnue ou arret explicite
            MotDriver.Motors[LEFT].stopMotor();
            MotDriver.Motors[RIGHT].stopMotor();
            break;
    }
}

struct CabinData {
    uint16_t distance_angle_1; // bits[15:2]=distance*4, bits[1:0]=angle_offset1 (2 bits bas)
    uint16_t distance_angle_2; // idem pour le 2e point
    uint8_t  offset_angles_q3; // bits[3:0]=angle_offset1 (4 bits hauts), bits[7:4]=angle_offset2 (4 bits hauts)
};

void parseExpressPacket(uint8_t *packet) {
    // Header (4 premiers octets) : sync (2x4bits) + checksum + start_angle_q6 (2 octets, little endian)
    uint16_t start_angle_raw = packet[2] | ((packet[3] & 0x7F) << 8);
    float start_angle = start_angle_raw / 64.0; // format q6
    bool start_flag = (packet[3] & 0x80) != 0; // bit S = début d'un nouveau tour complet

    Serial.print("=== Paquet | start_angle=");
    Serial.print(start_angle);
    Serial.print("° | nouveau tour=");
    Serial.println(start_flag ? "OUI" : "non");

    for (int i = 0; i < 16; i++) {
        CabinData *cabin = (CabinData*)(packet + 4 + i * 5);

        uint16_t dist1_raw = cabin->distance_angle_1 >> 2;
        uint16_t dist2_raw = cabin->distance_angle_2 >> 2;
        float distance1 = dist1_raw / 4.0; // mm
        float distance2 = dist2_raw / 4.0;

        int angle_offset1 = (cabin->distance_angle_1 & 0x03) | ((cabin->offset_angles_q3 & 0x0F) << 2);
        int angle_offset2 = (cabin->distance_angle_2 & 0x03) | ((cabin->offset_angles_q3 >> 4) << 2);
        float d_theta1 = angle_offset1 / 8.0; // q3 fixed point -> degrés
        float d_theta2 = angle_offset2 / 8.0;

        float angle1 = start_angle + d_theta1;
        float angle2 = start_angle + d_theta2;

        // Normaliser entre 0 et 360°
        if (angle1 < 0) angle1 += 360;
        if (angle1 >= 360) angle1 -= 360;
        if (angle2 < 0) angle2 += 360;
        if (angle2 >= 360) angle2 -= 360;

        Serial.print("  cabin[");
        Serial.print(i);
        Serial.print("] pt1: angle=");
        Serial.print(angle1);
        Serial.print("° dist=");
        Serial.print(distance1);
        Serial.print("mm | pt2: angle=");
        Serial.print(angle2);
        Serial.print("° dist=");
        Serial.print(distance2);
        Serial.println("mm");
    }
}

void loop() {
    // Serial.println(WiFi.softAPIP());

    uint8_t buf[1000];
    buf[999] = 0;
    WiFiClient client = server.available();
    static bool start = false;
    if (!start)
    {
        delay(2000);
        LidCtl.startExpressScan();
        start = true;
    }
    size_t read_bytes = 0;
    if (Serial2.available() >= 84)
    {
        read_bytes = Serial2.readBytes(buf, 84);
        parseExpressPacket(buf);
    }

    // for (int i = 0; i < read_bytes; i++) {
    //     Serial.print(buf[i], HEX);
    //     Serial.print(" ");
    // }
    // LidCtl.generate_request_packet(0x82, (uint8_t *) buf, 0);
    Serial.println();
    memset(buf, 0, read_bytes);
    unsigned long lastKeyTime = millis();
    bool motorsStopped = false;
    while (client.connected())
    {
        if (client.available()) {
            char c = client.read();
            // Serial.println(c);
            input_key(c);
            lastKeyTime = millis();
            motorsStopped = false;
        }
        else if (!motorsStopped && millis() - lastKeyTime > KEY_TIMEOUT_MS) {
            // securite : plus aucune touche recue, on coupe les moteurs
            input_key(0);
            motorsStopped = true;
        }
    }
    delay(500);
}

//     pinMode(encoderA, INPUT_PULLUP);
//     pinMode(encoderB, INPUT_PULLUP);

//     attachInterrupt(
//     digitalPinToInterrupt(encoderA),
//     encoderISR,
//     CHANGE
//   );