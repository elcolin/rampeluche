#include <Arduino.h>
#include "DriverMotor.h"


DriverMotor MotDriver;

void setup() {

    MotDriver.setupDriver();
    Serial.begin(115200);
}


void loop() {

    MotDriver.Motors[LEFT].setMotorForward(25);
    MotDriver.Motors[RIGHT].setMotorForward(25);

    delay(1000);

    MotDriver.Motors[LEFT].setMotorBackward(25);
    MotDriver.Motors[RIGHT].setMotorBackward(25);

    delay(1000);


}


// // Set LED_BUILTIN if it is not defined by Arduino framework
// #ifndef LED_BUILTIN
//     #define LED_BUILTIN 2
// #endif

// void setup()
// {
//   // initialize LED digital pin as an output.
//   pinMode(LED_BUILTIN, OUTPUT);
// }

// void loop()
// {
//   // turn the LED on (HIGH is the voltage level)
//   digitalWrite(LED_BUILTIN, HIGH);
//   // wait for a second
//   delay(500);
//   // turn the LED off by making the voltage LOW
//   digitalWrite(LED_BUILTIN, LOW);
//    // wait for a second
//   delay(500);
// }

// const int PIN_36 = 11;
// const int PIN_37 = 12;

// volatile long encoderCount = 0;

// const uint8_t encoderA = 36;
// const uint8_t encoderB = 37;

// void IRAM_ATTR encoderISR() {
//   bool A = digitalRead(encoderA);
//   bool B = digitalRead(encoderB);

//   if (A == B) {
//     encoderCount++;
//   } else {
//     encoderCount--;
//   }
// }

// void setup() {
//   Serial.begin(115200);

//   pinMode(encoderA, INPUT);
//   pinMode(encoderB, INPUT);
// }

// void loop() {
//   Serial.print(digitalRead(encoderA));
//   Serial.print(" ");
//   Serial.println(digitalRead(encoderB));
// }