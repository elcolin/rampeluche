#include "KeyboardControl.hpp"

DriveCommand decodeKey(char key)
{
    switch (key)
    {
        case 'w':
            return DriveCommand{
                MotorCommand{MotorAction::Forward, FORWARD_SPEED},
                MotorCommand{MotorAction::Forward, FORWARD_SPEED}
            };
        case 's':
            return DriveCommand{
                MotorCommand{MotorAction::Backward, BACKWARD_SPEED},
                MotorCommand{MotorAction::Backward, BACKWARD_SPEED}
            };
        case 'a': // pivot vers la gauche
            return DriveCommand{
                MotorCommand{MotorAction::Backward, TURN_SPEED},
                MotorCommand{MotorAction::Forward, TURN_SPEED}
            };
        case 'd': // pivot vers la droite
            return DriveCommand{
                MotorCommand{MotorAction::Forward, TURN_SPEED},
                MotorCommand{MotorAction::Backward, TURN_SPEED}
            };
        default: // touche inconnue ou arret explicite
            return DriveCommand{
                MotorCommand{MotorAction::Stop, 0},
                MotorCommand{MotorAction::Stop, 0}
            };
    }
}

bool keyTimedOut(unsigned long now, unsigned long lastKeyTime, unsigned long timeoutMs)
{
    return (now - lastKeyTime) > timeoutMs;
}
