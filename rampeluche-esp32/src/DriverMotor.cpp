#include "DriverMotor.h"

DriverMotor::DriverMotor(): Motors{
        Motor(AIN1, AIN2, APWM),
        Motor(BIN1, BIN2, BPWM)
      }
{
}

void DriverMotor::setupDriver()
{
    pinMode(STBY, OUTPUT);

    Motors[LEFT].setupMotor();
    Motors[RIGHT].setupMotor();

    digitalWrite(STBY, HIGH);
    Serial.println("Motor driver initialized");
}