#include "Motor.hpp"

Motor::Motor(const uint8_t IN1, const uint8_t IN2, const uint8_t PWM) : m_speed(0), m_IN1(IN1), m_IN2(IN2), m_PWM(PWM)
{
}


void Motor::setupMotor()
{
    pinMode(m_IN1, OUTPUT);
    pinMode(m_IN2, OUTPUT);
    pinMode(m_PWM, OUTPUT);
}
void Motor::stopMotor()
{
    digitalWrite(m_IN1, LOW);
    digitalWrite(m_IN2, LOW);
}

void Motor::setMotorForward(uint8_t per_speed)
{
    uint8_t speed = per_speed * 255 / 100;
    analogWrite(m_PWM, speed);


    digitalWrite(m_IN1, HIGH);
    digitalWrite(m_IN2, LOW);
}

void Motor::setMotorBackward(uint8_t per_speed)
{
    uint8_t speed = per_speed * 255 / 100;
    analogWrite(m_PWM, speed);

    digitalWrite(m_IN1, LOW);
    digitalWrite(m_IN2, HIGH);
}