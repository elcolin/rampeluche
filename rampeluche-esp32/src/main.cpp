/*
 * Blink
 * Turns on an LED on for one second,
 * then off for one second, repeatedly.
 */

#include <Arduino.h>

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


#define BIN1 39
#define BIN2 40
#define BPWM 41

#define AIN1 37
#define AIN2 36
#define APWM 35

#define STBY 38

// void setup() {
//   pinMode(IN1, OUTPUT);
//   pinMode(IN2, OUTPUT);
//   pinMode(PWM, OUTPUT);

// }

void setup() {
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);
  pinMode(BPWM, OUTPUT);

  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(APWM, OUTPUT);
  pinMode(STBY, OUTPUT);

  Serial.begin(115200);

  Serial.println("Motor driver initialized");
  // digitalWrite(STBY, HIGH);
}

#include <algorithm>



void loop() {
  // Serial.println("HIGH HIGH");
  // forwardB(50);
  // delay(2000);

  // backwardB(25);
  // delay(2000);

  // Forward B
  Serial.println("LOW HIGH");
  analogWrite(BPWM, 50);
  digitalWrite(STBY, HIGH);

  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, HIGH);

  delay(2000);

  // // Backward B
  // Serial.println("HIGH LOW");
  // digitalWrite(BIN1, HIGH);
  // digitalWrite(BIN2, LOW);

    // Forward A
  Serial.println("LOW HIGH");
  analogWrite(APWM, 50);
  digitalWrite(STBY, HIGH);

  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);

  delay(2000);

  // Backward A
  // Serial.println("HIGH LOW");
  // digitalWrite(AIN1, HIGH);
  // digitalWrite(AIN2, LOW);

  // analogWrite(BPWM, 50);
  delay(1000);

}