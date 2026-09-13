#pragma once

/*
 * H-SW CONTROL FUNCTION
 *
 * Inputs                     Outputs
 * +-----+-----+-----+------+-------+-------+---------------+
 * | IN1 | IN2 | PWM | STBY | OUT1  | OUT2  | Mode          |
 * +-----+-----+-----+------+-------+-------+---------------+
 * |  H  |  H  | H/L |  H   |   L   |   L   | Short brake   |
 * |  H  |  L  |  H  |  H   |  CCW  |       | Counterclock. |
 * |  H  |  L  |  L  |  H   |   L   |   L   | Short brake   |
 * |  L  |  H  |  H  |  H   |  CW   |       | Clockwise     |
 * |  L  |  H  |  L  |  H   |   L   |   L   | Short brake   |
 * |  L  |  L  |  H  |  H   |  OFF  |  OFF  | Stop          |
 * | H/L | H/L | H/L |  L   |  OFF  |  OFF  | Standby       |
 * +-----+-----+-----+------+-------+-------+---------------+
 *
 * Legend:
 *   H   = HIGH
 *   L   = LOW
 *   H/L = Don't care
 *   OFF = High impedance (Hi-Z)
 *
 * Direction:
 *   IN1=LOW  IN2=HIGH -> CW
 *   IN1=HIGH IN2=LOW  -> CCW
 */

#include <Arduino.h>
#include "Motor.hpp"
#include "Side.hpp"

class DriverMotor {
    public:
        DriverMotor();
        ~DriverMotor() = default;
        Motor   Motors[MOT_NUM];

        void setupDriver();
};