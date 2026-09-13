#include "EncoderController.hpp"
#include "pins.hpp"

EncoderController::EncoderController(): Encoders{
        Encoder(encoderLeftA, encoderLeftB),
        Encoder(encoderRightA, encoderRightB)
      }
{
}

void EncoderController::setup()
{
    Encoders[LEFT].setupEncoder();
    Encoders[RIGHT].setupEncoder();

    Serial.println("Encoders initialized");
}
