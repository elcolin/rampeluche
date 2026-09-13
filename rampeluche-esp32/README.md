# Rampeluche ESP32

Firmware embarqué du robot Rampeluche : un ESP32-S3 pilote deux moteurs DC
(traction différentielle) et un LIDAR RPLidar, et se laisse téléopérer au
clavier via Wi-Fi (SoftAP + socket TCP brut).

Voir le [README du projet global](../README.md) pour le contexte matériel
(châssis, moteurs, datasheets).

## Vue d'ensemble

```
tools/keyboard_client.py          (PC, Wi-Fi)
        | touche w/a/s/d, toutes les 150 ms, via TCP:1234
        v
   ESP32 SoftAP "ESP32_Control"
        |
   src/main.cpp  (boucle principale)
        |-- lit une touche client -> KeyboardControl::decodeKey()
        |-- decodeKey() -> DriveCommand (gauche/droite)
        |-- applyMotorCommand() -> DriverMotor -> Motor -> pins PWM/IN1/IN2
        |
        `-- lit le LIDAR sur Serial2 -> parseExpressPacket() -> Serial (debug)
```

Le robot n'exécute aucune logique de navigation autonome à ce stade : c'est
une téléopération manuelle. Le LIDAR est câblé et initialisé, ses trames sont
décodées, mais uniquement pour être affichées sur le port série (pas encore
exploitées pour éviter les obstacles ou faire du SLAM).

## Structure du code

```
include/            Headers partagés (déclarations de classes, pins)
  pins.hpp            Table de correspondance broches ESP32 <-> matériel
  DriverMotor.h       Regroupe les 2 moteurs (gauche/droite)
  Motor.hpp           Pilotage d'un moteur DC via pont en H
  LidarController.hpp Protocole série RPLidar (commandes, scan express)
  WifiHandler.hpp     Classe vide, non utilisée pour l'instant (voir "Dette")

src/                 Implémentation, dépend d'Arduino.h / matériel réel
  main.cpp            setup()/loop() : Wi-Fi, serveur TCP, boucle de pilotage,
                       lecture + parsing des trames LIDAR
  DriverMotor.cpp
  Motor.cpp
  LidarController.cpp

lib/KeyboardControl/  Logique pure de décision "touche -> commande moteur"
  KeyboardControl.hpp   Types (MotorAction, MotorCommand, DriveCommand),
                         constantes de vitesse/timeout, decodeKey(), keyTimedOut()
  KeyboardControl.cpp   Implémentation, aucune dépendance Arduino/matériel

test/test_keyboard_control/
  test_main.cpp        Tests unitaires (Unity) de decodeKey()/keyTimedOut(),
                        exécutés hors cible (pas besoin d'ESP32)

tools/
  keyboard_client.py    Client Python (PC) : capte w/a/s/d au clavier et les
                         envoie en boucle au robot par TCP
  requirements.txt

platformio.ini        2 environnements : esp32-s3-devkitc-1 (firmware réel)
                       et native (tests unitaires sur machine de dev)
```

## Choix d'architecture

### Séparer la logique pure du code matériel

`lib/KeyboardControl` ne dépend que de `<cstdint>`, pas d'`Arduino.h`. La
fonction `decodeKey()` (touche -> commandes moteur gauche/droite) et
`keyTimedOut()` (calcul du délai de sécurité) sont des fonctions pures,
sans effet de bord matériel. Ça permet de les tester unitairement sur la
machine de dev, sans flasher un ESP32 à chaque changement, via
l'environnement `native` de PlatformIO :

```sh
pio test -e native
```

`src/main.cpp` reste le seul endroit qui connecte cette logique au matériel
réel (`applyMotorCommand()` traduit un `MotorCommand` en appels
`Motor::setMotorForward/Backward/stopMotor`). C'est le seul module du
firmware couvert par des tests aujourd'hui ; le reste (moteurs, LIDAR,
Wi-Fi) n'est vérifiable qu'en conditions réelles, faute d'abstraction
matérielle testable.

### Coupure de sécurité si le client décroche

`main.cpp` mémorise l'horodatage de la dernière touche reçue. Si aucune
touche n'arrive pendant `KEY_TIMEOUT_MS` (500 ms, défini dans
`KeyboardControl.hpp`), la boucle appelle `input_key(0)` pour arrêter les
deux moteurs. Ça protège contre une perte de connexion Wi-Fi ou un client
qui plante avec le robot lancé. Côté client Python, l'envoi est cadencé à
150 ms (`SEND_INTERVAL`), volontairement bien en dessous du timeout
firmware, pour qu'un simple relâchement de touche coupe les moteurs de
façon réactive sans attendre le timeout de sécurité.

### Wi-Fi SoftAP + TCP brut, pas de protocole applicatif

L'ESP32 monte son propre point d'accès (`ESP32_Control`) et un
`WiFiServer` TCP sur le port 1234. Le protocole est volontairement minimal :
un caractère par commande (`w`/`a`/`s`/`d`, tout le reste = arrêt), pas de
framing ni de librairie réseau tierce. Suffisant pour une téléopération
directe ; pas conçu pour survivre à plusieurs clients simultanés ou à des
pertes de paquets (TCP masque une partie du problème, mais rien ne gère une
reconnexion propre du serveur après déconnexion du client).

### Contrôle moteur : pont en H, vitesse en pourcentage

`Motor` pilote un pont en H classique (2 entrées logiques IN1/IN2 + 1 PWM,
table de vérité documentée en commentaire dans `DriverMotor.h`). L'API
publique (`setMotorForward/Backward(uint8_t per_speed)`) prend un
pourcentage 0-100 plutôt qu'une valeur PWM brute, pour découpler l'appelant
(`KeyboardControl`, qui définit `FORWARD_SPEED`/`BACKWARD_SPEED`/
`TURN_SPEED` en pourcentage) de la résolution PWM réelle du driver.
`DriverMotor` regroupe les deux `Motor` (gauche/droite) et gère la broche
`STBY` commune aux deux ponts.

### LIDAR : décodage du protocole RPLidar "express scan"

`LidarController` construit les trames de commande RPLidar (en-tête `0xA5`,
checksum XOR) et démarre un scan express sur `Serial2`. `main.cpp` lit les
paquets de 84 octets et les décode (`parseExpressPacket`) : angle de départ
en virgule fixe Q6, 16 "cabines" par paquet contenant chacune 2 points
(distance en Q2 + offset d'angle en Q3), selon le format propriétaire
Slamtec. Ce parsing est actuellement câblé pour finir en `Serial.print` de
debug — aucune structure de données exploitable par le reste du firmware
n'est produite pour l'instant (pas encore de nuage de points en mémoire).

## Table des broches (`include/pins.hpp`)

| Broche ESP32 | Fonction |
|---|---|
| 11 | LIDAR : contrôle moteur (PWM) |
| 12 | LIDAR : RX (UART Serial2) |
| 13 | LIDAR : TX (UART Serial2) |
| 39 / 40 / 41 | Moteur droit : IN1 / IN2 / PWM |
| 37 / 36 / 35 | Moteur gauche : IN1 / IN2 / PWM |
| 38 | STBY commun aux deux ponts en H |

## Compiler, flasher, tester

```sh
# Firmware réel (ESP32-S3), depuis rampeluche-esp32/
pio run -e esp32-s3-devkitc-1
pio run -e esp32-s3-devkitc-1 -t upload
pio device monitor -b 115200

# Tests unitaires de lib/KeyboardControl, sur la machine de dev (pas d'ESP32 requis)
pio test -e native
```

Sur macOS, si `pio test -e native` échoue au link avec une erreur du type
`archive member '/' not a mach-o file`, c'est qu'un `ar` GNU (ex: via
Homebrew binutils) passe avant celui d'Apple dans le `PATH`. Contournement :

```sh
PATH="/usr/bin:$PATH" pio test -e native
```

### Piloter le robot

```sh
cd tools
pip install -r requirements.txt
python3 keyboard_client.py            # se connecte à 192.168.4.1:1234 par défaut
```

Touches : `w` avancer, `s` reculer, `a`/`d` pivoter sur place, relâcher
= arrêt, `q`/Échap pour quitter.

## Dette technique connue

- `include/WifiHandler.hpp` est une classe vide, non instanciée : la
  logique Wi-Fi vit directement dans `main.cpp` (setup du SoftAP,
  `WiFiServer`). À supprimer ou à finir d'implémenter.
- Le parsing LIDAR (`parseExpressPacket` dans `main.cpp`) ne fait que
  logger sur `Serial` ; il n'y a pas encore de consommateur de ces données
  (pas d'évitement d'obstacle, pas de SLAM).
- Du code d'encodeur odométrique (interruption `encoderISR`, comptage de
  ticks) est présent dans `main.cpp` mais commenté / non branché.
- `DriverMotor.h` utilise l'extension `.h` alors que le reste des headers
  du projet utilise `.hpp` — incohérence mineure de nommage.
