#pragma once

#include "KeyboardControl.hpp"

// Etat d'une session teleop TCP : decode chaque octet recu en DriveCommand
// (via KeyboardControl::decodeKey) et applique la coupure de securite
// moteur des que plus aucune touche n'arrive pendant KEY_TIMEOUT_MS.
//
// Logique pure, sans WiFiServer/WiFiClient ici, pour rester testable en
// environnement native (voir test/test_wifi_teleop_server). Le wrapper
// Arduino (src/main.cpp) reste fin : il lit les octets sur le socket TCP
// et pousse chacun a onKeyReceived(), ou appelle checkTimeout() quand rien
// n'est disponible.
class WifiTeleopServer {
public:
    struct TimeoutResult {
        bool          triggered; // vrai la premiere fois que le timeout est detecte
        DriveCommand  command;   // commande d'arret a appliquer si triggered
    };

    explicit WifiTeleopServer(unsigned long now);

    // Touche recue sur le socket TCP : reinitialise le timeout et retourne
    // la commande moteur correspondante.
    DriveCommand onKeyReceived(char key, unsigned long now);

    // A appeler quand aucune donnee n'est disponible sur le socket. Ne
    // remonte le stop de securite qu'une seule fois par periode de silence
    // (evite de marteler le driver moteur d'ordres Stop redondants).
    TimeoutResult checkTimeout(unsigned long now);

private:
    unsigned long m_lastKeyTime;
    bool          m_motorsStopped;
};
