#pragma once

#include <Arduino.h>

// Decodeur quadrature x4 pour encodeur 2 canaux (A/B) type N20.
//
// A chaque front (montant ou descendant) sur A ou B, l'etat courant des deux
// canaux est compare au precedent pour determiner si la roue a avance ou
// recule d'un quart de cran, via une table de transition. Une transition
// invalide (saut de 2 crans, du bruit ou un front rate) compte pour 0 plutot
// que de fausser le comptage dans un sens arbitraire.
class Encoder {
    public:
        Encoder(const uint8_t pinA, const uint8_t pinB);
        ~Encoder() = default;

        void setupEncoder();
        long getCount() const;
        void reset();

    private:
        static void IRAM_ATTR isrTrampoline(void *arg);
        void IRAM_ATTR handleInterrupt();

        const uint8_t   m_pinA;
        const uint8_t   m_pinB;
        volatile long   m_count;
        volatile uint8_t m_lastState;
};
