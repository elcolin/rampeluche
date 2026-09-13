#include "WifiTeleopServer.hpp"

WifiTeleopServer::WifiTeleopServer(unsigned long now)
    : m_lastKeyTime(now)
    , m_motorsStopped(false)
{
}

DriveCommand WifiTeleopServer::onKeyReceived(char key, unsigned long now)
{
    m_lastKeyTime = now;
    m_motorsStopped = false;
    return decodeKey(key);
}

WifiTeleopServer::TimeoutResult WifiTeleopServer::checkTimeout(unsigned long now)
{
    if (m_motorsStopped || !keyTimedOut(now, m_lastKeyTime, KEY_TIMEOUT_MS)) {
        return TimeoutResult{false, DriveCommand{}};
    }

    m_motorsStopped = true;
    return TimeoutResult{
        true,
        DriveCommand{
            MotorCommand{MotorAction::Stop, 0},
            MotorCommand{MotorAction::Stop, 0}
        }
    };
}
