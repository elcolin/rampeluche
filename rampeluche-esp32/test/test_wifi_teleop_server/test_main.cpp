#include <unity.h>
#include "WifiTeleopServer.hpp"

void setUp() {}
void tearDown() {}

static void assertCommand(const MotorCommand &cmd, MotorAction action, uint8_t speed)
{
    TEST_ASSERT_EQUAL(static_cast<int>(action), static_cast<int>(cmd.action));
    if (action != MotorAction::Stop) {
        TEST_ASSERT_EQUAL_UINT8(speed, cmd.speed);
    }
}

void test_on_key_received_decodes_the_key_into_a_drive_command()
{
    WifiTeleopServer session(1000);
    DriveCommand cmd = session.onKeyReceived('w', 1000);
    assertCommand(cmd.left, MotorAction::Forward, FORWARD_SPEED);
    assertCommand(cmd.right, MotorAction::Forward, FORWARD_SPEED);
}

void test_no_timeout_right_after_construction()
{
    WifiTeleopServer session(1000);
    WifiTeleopServer::TimeoutResult result = session.checkTimeout(1000);
    TEST_ASSERT_FALSE(result.triggered);
}

void test_no_timeout_before_threshold_elapsed()
{
    WifiTeleopServer session(1000);
    WifiTeleopServer::TimeoutResult result = session.checkTimeout(1000 + KEY_TIMEOUT_MS);
    TEST_ASSERT_FALSE(result.triggered);
}

void test_timeout_triggers_a_motor_stop_command_once_threshold_exceeded()
{
    WifiTeleopServer session(1000);
    WifiTeleopServer::TimeoutResult result = session.checkTimeout(1000 + KEY_TIMEOUT_MS + 1);
    TEST_ASSERT_TRUE(result.triggered);
    assertCommand(result.command.left, MotorAction::Stop, 0);
    assertCommand(result.command.right, MotorAction::Stop, 0);
}

void test_timeout_is_reported_only_once_until_a_new_key_arrives()
{
    // Une fois les moteurs coupes, on evite de renvoyer le stop en boucle
    // (pas d'action redondante sur le driver moteur tant qu'aucune touche
    // n'est revenue).
    WifiTeleopServer session(1000);
    session.checkTimeout(1000 + KEY_TIMEOUT_MS + 1);
    WifiTeleopServer::TimeoutResult second = session.checkTimeout(1000 + KEY_TIMEOUT_MS + 2);
    TEST_ASSERT_FALSE(second.triggered);
}

void test_new_key_after_timeout_reactivates_timeout_detection()
{
    WifiTeleopServer session(1000);
    session.checkTimeout(1000 + KEY_TIMEOUT_MS + 1);
    session.onKeyReceived('w', 2000);

    WifiTeleopServer::TimeoutResult stillDriving = session.checkTimeout(2000 + KEY_TIMEOUT_MS);
    TEST_ASSERT_FALSE(stillDriving.triggered);

    WifiTeleopServer::TimeoutResult timedOutAgain = session.checkTimeout(2000 + KEY_TIMEOUT_MS + 1);
    TEST_ASSERT_TRUE(timedOutAgain.triggered);
}

int main(int argc, char **argv)
{
    UNITY_BEGIN();
    RUN_TEST(test_on_key_received_decodes_the_key_into_a_drive_command);
    RUN_TEST(test_no_timeout_right_after_construction);
    RUN_TEST(test_no_timeout_before_threshold_elapsed);
    RUN_TEST(test_timeout_triggers_a_motor_stop_command_once_threshold_exceeded);
    RUN_TEST(test_timeout_is_reported_only_once_until_a_new_key_arrives);
    RUN_TEST(test_new_key_after_timeout_reactivates_timeout_detection);
    return UNITY_END();
}
