#include <climits>
#include <unity.h>
#include "KeyboardControl.hpp"

void setUp() {}
void tearDown() {}

static void assertCommand(const MotorCommand &cmd, MotorAction action, uint8_t speed)
{
    TEST_ASSERT_EQUAL(static_cast<int>(action), static_cast<int>(cmd.action));
    if (action != MotorAction::Stop) {
        TEST_ASSERT_EQUAL_UINT8(speed, cmd.speed);
    }
}

void test_forward_key_drives_both_wheels_forward()
{
    DriveCommand cmd = decodeKey('w');
    assertCommand(cmd.left, MotorAction::Forward, FORWARD_SPEED);
    assertCommand(cmd.right, MotorAction::Forward, FORWARD_SPEED);
}

void test_backward_key_drives_both_wheels_backward()
{
    DriveCommand cmd = decodeKey('s');
    assertCommand(cmd.left, MotorAction::Backward, BACKWARD_SPEED);
    assertCommand(cmd.right, MotorAction::Backward, BACKWARD_SPEED);
}

void test_left_key_pivots_in_place_towards_the_left()
{
    DriveCommand cmd = decodeKey('a');
    assertCommand(cmd.left, MotorAction::Backward, TURN_SPEED);
    assertCommand(cmd.right, MotorAction::Forward, TURN_SPEED);
}

void test_right_key_pivots_in_place_towards_the_right()
{
    DriveCommand cmd = decodeKey('d');
    assertCommand(cmd.left, MotorAction::Forward, TURN_SPEED);
    assertCommand(cmd.right, MotorAction::Backward, TURN_SPEED);
}

void test_unknown_key_stops_both_wheels()
{
    DriveCommand cmd = decodeKey('z');
    assertCommand(cmd.left, MotorAction::Stop, 0);
    assertCommand(cmd.right, MotorAction::Stop, 0);
}

void test_null_key_stops_both_wheels()
{
    // Utilise en interne par main.cpp comme signal explicite d'arret
    // (timeout de securite quand plus aucune touche n'arrive).
    DriveCommand cmd = decodeKey('\0');
    assertCommand(cmd.left, MotorAction::Stop, 0);
    assertCommand(cmd.right, MotorAction::Stop, 0);
}

void test_no_timeout_when_key_received_recently()
{
    TEST_ASSERT_FALSE(keyTimedOut(1000, 900, KEY_TIMEOUT_MS));
}

void test_timeout_when_delay_exceeds_threshold()
{
    TEST_ASSERT_TRUE(keyTimedOut(2000, 1000, KEY_TIMEOUT_MS));
}

void test_timeout_boundary_is_exclusive()
{
    // Exactement au seuil : pas encore en timeout (strictement superieur requis).
    TEST_ASSERT_FALSE(keyTimedOut(1000 + KEY_TIMEOUT_MS, 1000, KEY_TIMEOUT_MS));
    TEST_ASSERT_TRUE(keyTimedOut(1000 + KEY_TIMEOUT_MS + 1, 1000, KEY_TIMEOUT_MS));
}

void test_timeout_survives_millis_wraparound()
{
    // millis() reboucle a 0 apres ULONG_MAX ; la soustraction non signee doit
    // rester correcte dans ce cas (idiome standard Arduino).
    unsigned long lastKeyTime = ULONG_MAX - 10;
    unsigned long now = 5; // a rebouclee, 16ms plus tard en realite
    TEST_ASSERT_FALSE(keyTimedOut(now, lastKeyTime, KEY_TIMEOUT_MS));
}

int main(int argc, char **argv)
{
    UNITY_BEGIN();
    RUN_TEST(test_forward_key_drives_both_wheels_forward);
    RUN_TEST(test_backward_key_drives_both_wheels_backward);
    RUN_TEST(test_left_key_pivots_in_place_towards_the_left);
    RUN_TEST(test_right_key_pivots_in_place_towards_the_right);
    RUN_TEST(test_unknown_key_stops_both_wheels);
    RUN_TEST(test_null_key_stops_both_wheels);
    RUN_TEST(test_no_timeout_when_key_received_recently);
    RUN_TEST(test_timeout_when_delay_exceeds_threshold);
    RUN_TEST(test_timeout_boundary_is_exclusive);
    RUN_TEST(test_timeout_survives_millis_wraparound);
    return UNITY_END();
}
