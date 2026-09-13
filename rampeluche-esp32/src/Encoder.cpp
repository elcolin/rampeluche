#include "Encoder.hpp"

// Table de transition quadrature x4, indexee par (etat_precedent << 2 | etat_courant)
// ou chaque etat vaut (A << 1 | B). Valeur = +1 (avant), -1 (arriere) ou 0 (invalide/immobile).
static const int8_t QUAD_TABLE[16] = {
     0, -1,  1,  0,
     1,  0,  0, -1,
    -1,  0,  0,  1,
     0,  1, -1,  0
};

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
    uint8_t index = (m_lastState << 2) | state;
    m_count += QUAD_TABLE[index];
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
