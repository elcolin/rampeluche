#include <Arduino.h>
#include "DriverMotor.h"
#include <WiFi.h>

const uint8_t encoderA = 15;
const uint8_t encoderB = 16;
DriverMotor MotDriver;

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

//     pinMode(encoderA, INPUT_PULLUP);
//     pinMode(encoderB, INPUT_PULLUP);

//     attachInterrupt(
//     digitalPinToInterrupt(encoderA),
//     encoderISR,
//     CHANGE
//   );
    WiFi.softAP(
        "ESP32_Control",
        "12345678"
    );

    Serial.println(WiFi.softAPIP());

    server.begin();
    MotDriver.setupDriver();
    // Serial.begin(115200);
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
            speed_left = 1.5;
            speed_right = 1.5;
            direction = 1;

            break;
        case 's':
            speed_left = -1.25;
            speed_right = -1.25;
            direction = -1;
            break;
        case 'a':
            speed_left = -std::copysign(0.90, direction);
            speed_right = std::copysign(0.90, direction);

            // speed_left = -0.90;
            // speed_right = 0.90;


            break;
        case 'd':
            // speed_left = 0.90;
            // speed_right = -0.90;

            speed_left = std::copysign(0.90, direction);
            speed_right = -std::copysign(0.90, direction);

            break;

        default:
            direction = 0;
            speed_right = 0;
            speed_left = 0;

            MotDriver.Motors[LEFT].stopMotor();
            MotDriver.Motors[RIGHT].stopMotor();
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
    Serial.println(WiFi.softAPIP());

    WiFiClient client = server.available();

    if (!client)
        return;
    while(client.connected()){
        // Serial.println("Connected to client");
        if (client.available()) {
            char c = client.read();
            Serial.println(c);
            input_key(c);
        }
    }
}