#pragma once

#include <cstdint>

// Vitesses (pourcentage de PWM, 0-100) appliquees selon la touche recue.
constexpr uint8_t FORWARD_SPEED  = 70;
constexpr uint8_t BACKWARD_SPEED = 35;
constexpr uint8_t TURN_SPEED     = 35;

// Duree max (ms) sans recevoir de touche avant coupure de securite des moteurs.
constexpr unsigned long KEY_TIMEOUT_MS = 500;

enum class MotorAction : uint8_t {
    Forward,
    Backward,
    Stop
};

struct MotorCommand {
    MotorAction action;
    uint8_t     speed; // pourcentage 0-100, ignore si action == Stop
};

struct DriveCommand {
    MotorCommand left;
    MotorCommand right;
};

// Traduit une touche clavier (w/a/s/d, ou toute autre valeur -> arret) en
// commande moteur gauche/droite. Fonction pure, sans dependance materielle :
// testable hors ESP32 (voir test/test_keyboard_control).
DriveCommand decodeKey(char key);

// Indique si le delai depuis la derniere touche recue depasse le timeout de
// securite. Soustraction non signee volontaire pour rester correcte meme en
// cas de wraparound de millis() (idiome standard Arduino).
bool keyTimedOut(unsigned long now, unsigned long lastKeyTime, unsigned long timeoutMs);
