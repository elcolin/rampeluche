#include <unity.h>
#include "QuadratureDecoder.hpp"

void setUp() {}
void tearDown() {}

// Etats des canaux A/B code en (A << 1 | B). Suite de Gray sur un cran complet :
// 00 -> 01 -> 11 -> 10 -> 00 (avant) et son inverse (arriere).
constexpr uint8_t S00 = 0b00;
constexpr uint8_t S01 = 0b01;
constexpr uint8_t S11 = 0b11;
constexpr uint8_t S10 = 0b10;

void test_forward_cycle_returns_plus_one()
{
    // 00 -> 10 -> 11 -> 01 -> 00, chaque pas d'un cran vers l'avant.
    TEST_ASSERT_EQUAL_INT8(1, quadratureDelta(S00, S10));
    TEST_ASSERT_EQUAL_INT8(1, quadratureDelta(S10, S11));
    TEST_ASSERT_EQUAL_INT8(1, quadratureDelta(S11, S01));
    TEST_ASSERT_EQUAL_INT8(1, quadratureDelta(S01, S00));
}

void test_backward_cycle_returns_minus_one()
{
    // Meme cycle parcouru en sens inverse : 00 -> 01 -> 11 -> 10 -> 00.
    TEST_ASSERT_EQUAL_INT8(-1, quadratureDelta(S00, S01));
    TEST_ASSERT_EQUAL_INT8(-1, quadratureDelta(S01, S11));
    TEST_ASSERT_EQUAL_INT8(-1, quadratureDelta(S11, S10));
    TEST_ASSERT_EQUAL_INT8(-1, quadratureDelta(S10, S00));
}

void test_no_change_returns_zero()
{
    TEST_ASSERT_EQUAL_INT8(0, quadratureDelta(S00, S00));
    TEST_ASSERT_EQUAL_INT8(0, quadratureDelta(S01, S01));
    TEST_ASSERT_EQUAL_INT8(0, quadratureDelta(S11, S11));
    TEST_ASSERT_EQUAL_INT8(0, quadratureDelta(S10, S10));
}

void test_invalid_two_bit_jump_returns_zero()
{
    // Saut diagonal (00<->11 ou 01<->10) : impossible en quadrature propre,
    // signe d'un front d'interruption rate ou de bruit. Ne doit pas fausser
    // le comptage dans un sens arbitraire.
    TEST_ASSERT_EQUAL_INT8(0, quadratureDelta(S00, S11));
    TEST_ASSERT_EQUAL_INT8(0, quadratureDelta(S11, S00));
    TEST_ASSERT_EQUAL_INT8(0, quadratureDelta(S01, S10));
    TEST_ASSERT_EQUAL_INT8(0, quadratureDelta(S10, S01));
}

void test_ignores_bits_above_the_two_state_bits()
{
    // Seuls les 2 bits de poids faible codent l'etat A/B ; le reste doit
    // etre ignore (garde-fou si un appelant ne masque pas ses valeurs).
    TEST_ASSERT_EQUAL_INT8(1, quadratureDelta(0xF0 | S00, 0xF0 | S10));
}

void test_accumulated_count_over_several_forward_turns()
{
    // Simule 3 tours complets du cycle avant (4 crans chacun) et verifie
    // que le compteur cumule bien +12, comme le ferait Encoder::handleInterrupt.
    const uint8_t sequence[] = {S00, S10, S11, S01, S00, S10, S11, S01, S00, S10, S11, S01, S00};
    long count = 0;
    for (size_t i = 1; i < sizeof(sequence); i++) {
        count += quadratureDelta(sequence[i - 1], sequence[i]);
    }
    TEST_ASSERT_EQUAL(12, count);
}

void test_accumulated_count_reverses_with_direction()
{
    // Deux crans avant, puis les deux memes crans en arriere : le compteur
    // doit revenir a 0.
    long count = 0;
    count += quadratureDelta(S00, S10); // +1
    count += quadratureDelta(S10, S11); // +1
    TEST_ASSERT_EQUAL(2, count);

    count += quadratureDelta(S11, S10); // -1
    count += quadratureDelta(S10, S00); // -1
    TEST_ASSERT_EQUAL(0, count);
}

int main(int argc, char **argv)
{
    UNITY_BEGIN();
    RUN_TEST(test_forward_cycle_returns_plus_one);
    RUN_TEST(test_backward_cycle_returns_minus_one);
    RUN_TEST(test_no_change_returns_zero);
    RUN_TEST(test_invalid_two_bit_jump_returns_zero);
    RUN_TEST(test_ignores_bits_above_the_two_state_bits);
    RUN_TEST(test_accumulated_count_over_several_forward_turns);
    RUN_TEST(test_accumulated_count_reverses_with_direction);
    return UNITY_END();
}
