#pragma once

#include <Arduino.h>

class Motor {
    public:
        Motor(const uint8_t IN1, const uint8_t IN2, const uint8_t PWM);
        ~Motor() = default;
        void stopMotor();
        void setupMotor();
        void setMotorForward(uint8_t per_speed);
        void setMotorBackward(uint8_t per_speed);
    private:
        uint8_t         m_speed;
        const uint8_t   m_IN1;
        const uint8_t   m_IN2;
        const uint8_t   m_PWM;
};