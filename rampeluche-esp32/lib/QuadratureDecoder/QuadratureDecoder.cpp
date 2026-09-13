#include "QuadratureDecoder.hpp"

// Table de transition quadrature x4, indexee par (etat_precedent << 2 | etat_courant)
// ou chaque etat vaut (A << 1 | B). Valeur = +1 (avant), -1 (arriere) ou 0 (invalide/immobile).
static const int8_t QUAD_TABLE[16] = {
     0, -1,  1,  0,
     1,  0,  0, -1,
    -1,  0,  0,  1,
     0,  1, -1,  0
};

int8_t quadratureDelta(uint8_t prevState, uint8_t currState)
{
    uint8_t index = ((prevState & 0x03) << 2) | (currState & 0x03);
    return QUAD_TABLE[index];
}
