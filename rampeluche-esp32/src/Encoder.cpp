#include "Encoder.hpp"
#include "QuadratureDecoder.hpp"

Encoder::Encoder(const uint8_t pinA, const uint8_t pinB)
    : m_pinA(pinA), m_pinB(pinB), m_count(0), m_lastState(0)
{
}

void Encoder::setupEncoder()
{
    pinMode(m_pinA, INPUT_PULLUP);
    pinMode(m_pinB, INPUT_PULLUP);

    m_lastState = (digitalRead(m_pinA) << 1) | digitalRead(m_pinB);

    attachInterruptArg(digitalPinToInterrupt(m_pinA), isrTrampoline, this, CHANGE);
    attachInterruptArg(digitalPinToInterrupt(m_pinB), isrTrampoline, this, CHANGE);
}

void IRAM_ATTR Encoder::isrTrampoline(void *arg)
{
    static_cast<Encoder *>(arg)->handleInterrupt();
}

void IRAM_ATTR Encoder::handleInterrupt()
{
    uint8_t state = (digitalRead(m_pinA) << 1) | digitalRead(m_pinB);
    m_count += quadratureDelta(m_lastState, state);
    m_lastState = state;
}

long Encoder::getCount() const
{
    return m_count;
}

void Encoder::reset()
{
    m_count = 0;
}
