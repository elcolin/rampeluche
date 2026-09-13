#include <Arduino.h>
#include "DriverMotor.h"
#include "pins.hpp"
#include "LidarController.hpp"
#include "RplidarDecoder.hpp"
#include "KeyboardControl.hpp"
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


static void applyMotorCommand(Motor &motor, const MotorCommand &cmd)
{
    switch (cmd.action)
    {
        case MotorAction::Forward:
            motor.setMotorForward(cmd.speed);
            break;
        case MotorAction::Backward:
            motor.setMotorBackward(cmd.speed);
            break;
        case MotorAction::Stop:
            motor.stopMotor();
            break;
    }
}

void input_key(char c)
{
    DriveCommand cmd = decodeKey(c);
    applyMotorCommand(MotDriver.Motors[LEFT], cmd.left);
    applyMotorCommand(MotDriver.Motors[RIGHT], cmd.right);
}

// Affiche les points fraichement decodes par LidCtl.poll() (cf. loop()).
// Remplace l'ancien parseExpressPacket()/CabinData ad-hoc : LidarController
// s'appuie desormais sur lib/RplidarDecoder, qui valide le checksum de
// chaque paquet et se resynchronise seul sur le flux UART si besoin (voir
// test/test_rplidar_decoder pour le detail du format).
void onLidarPoints(const Rplidar::ScanPoint *points, size_t count, void *user_data)
{
    for (size_t i = 0; i < count; i++) {
        Serial.print(points[i].angle_deg);
        Serial.print("\t");
        Serial.println(points[i].distance_mm);
    }
}

void loop() {
    // Serial.println(WiFi.softAPIP());

    WiFiClient client = server.available();
    static bool start = false;
    if (!start)
    {
        delay(2000);
        LidCtl.startExpressScan();
        start = true;
    }
    LidCtl.poll(onLidarPoints);

    unsigned long lastKeyTime = millis();
    bool motorsStopped = false;
    while (client.connected())
    {
        // Continuer a vider Serial2 pendant toute la session de pilotage :
        // sinon le petit buffer RX materiel deborde des qu'un client reste
        // connecte (le cas d'usage normal), et le decodage lidar degenere
        // en resynchronisations/erreurs de checksum en continu.
        LidCtl.poll(onLidarPoints);

        if (client.available()) {
            char c = client.read();
            // Serial.println(c);
            input_key(c);
            lastKeyTime = millis();
            motorsStopped = false;
        }
        else if (!motorsStopped && keyTimedOut(millis(), lastKeyTime, KEY_TIMEOUT_MS)) {
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