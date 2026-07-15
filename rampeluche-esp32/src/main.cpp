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


uint8_t speed = 35;

void input_key(char c)
{
    static double speed_left = 0;
    static double speed_right = 0;
    static int direction = 0;

    switch (c)
    {
        case 'w':
            speed_left = 2;
            speed_right = 2;
            direction = 1;

            break;
        case 's':
            speed_left = -1;
            speed_right = -1;
            direction = -1;
            break;
        case 'a':
            speed_left = -std::copysign(1, direction);
            speed_right = std::copysign(1, direction);

            break;
        case 'd':

            speed_left = std::copysign(1, direction);
            speed_right = -std::copysign(1, direction);

            break;

        default:
            direction = 0;
            speed_right = 0;
            speed_left = 0;
        break;

    }


    if (speed_right >= 0)
        MotDriver.Motors[RIGHT].setMotorForward(speed * speed_right);
    if (speed_right < 0)
        MotDriver.Motors[RIGHT].setMotorBackward(speed * std::abs(speed_right));
    if (speed_left >= 0)
        MotDriver.Motors[LEFT].setMotorForward(speed * speed_left);
    if (speed_left < 0)
        MotDriver.Motors[LEFT].setMotorBackward(speed * std::abs(speed_left));

}

void loop() {
    // Serial.println(WiFi.softAPIP());

    char buf[1000];
    buf[999] = 0;
    WiFiClient client = server.available();
    static bool start = false;
    if (!start)
    {
        delay(2000);
        LidCtl.startExpressScan();
        start = true;
    }

    int read_bytes = Serial2.readBytes(buf ,84);
    // Serial.println(read_bytes);

    for (int i = 0; i < read_bytes; i++) {
        Serial.print(buf[i], HEX);
        Serial.print(" ");
    }
    // LidCtl.generate_request_packet(0x82, (uint8_t *) buf, 0);
    // Serial.println();
    memset(buf, 0, 84);
    while (client.connected())
    {
        if (client.available()) {
            char c = client.read();
            // Serial.println(c);
            input_key(c);
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