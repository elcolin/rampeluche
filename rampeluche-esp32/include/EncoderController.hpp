#pragma once

#include <Arduino.h>
#include "Encoder.hpp"
#include "Side.hpp"

class EncoderController {
    public:
        EncoderController();
        ~EncoderController() = default;
        Encoder Encoders[MOT_NUM];

        void setup();
};
