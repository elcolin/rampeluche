#pragma once

#include <cstdint>

// Renvoie le delta (+1, -1 ou 0) correspondant a une transition d'encodeur
// quadrature entre deux etats successifs des canaux A/B, chacun code sur les
// 2 bits de poids faible (bit1=A, bit0=B). Fonction pure, sans dependance
// materielle : testable hors ESP32 (voir test/test_quadrature_decoder).
//
// 0 est renvoye pour une transition invalide (saut de 2 crans - bruit ou
// front rate) plutot que de fausser le comptage dans un sens arbitraire.
int8_t quadratureDelta(uint8_t prevState, uint8_t currState);
