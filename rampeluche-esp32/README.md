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
        `-- LidCtl.poll(onLidarPoints) -> Serial2 -> RplidarDecoder::StreamDecoder -> Serial (debug)
```

Le robot n'exécute aucune logique de navigation autonome à ce stade : c'est
une téléopération manuelle. Le LIDAR est câblé et initialisé, ses trames sont
décodées par `lib/RplidarDecoder`, mais uniquement pour être affichées sur le
port série (pas encore exploitées pour éviter les obstacles ou faire du
SLAM).

## Structure du code

```
include/            Headers partagés (déclarations de classes, pins)
  pins.hpp            Table de correspondance broches ESP32 <-> matériel
  DriverMotor.h       Regroupe les 2 moteurs (gauche/droite)
  Motor.hpp           Pilotage d'un moteur DC via pont en H
  LidarController.hpp Protocole série RPLidar (commandes, scan express) +
                       poll() qui lit Serial2 et restitue les points decodes
  WifiHandler.hpp     Classe vide, non utilisée pour l'instant (voir "Dette")

src/                 Implémentation, dépend d'Arduino.h / matériel réel
  main.cpp            setup()/loop() : Wi-Fi, serveur TCP, boucle de pilotage,
                       poll() du LIDAR (LidarController -> RplidarDecoder)
  DriverMotor.cpp
  Motor.cpp
  LidarController.cpp

lib/KeyboardControl/  Logique pure de décision "touche -> commande moteur"
  KeyboardControl.hpp   Types (MotorAction, MotorCommand, DriveCommand),
                         constantes de vitesse/timeout, decodeKey(), keyTimedOut()
  KeyboardControl.cpp   Implémentation, aucune dépendance Arduino/matériel

lib/RplidarDecoder/   Logique pure de décodage des paquets "Express Scan"
                      du RPLIDAR A2M8 (synchro, checksum, interpolation
                      angle/distance, resynchronisation sur un flux d'octets)
  RplidarDecoder.hpp    Types (ScanPoint), fonctions de décodage d'un paquet
                        (decodeCapsulePair()...), StreamDecoder (etat +
                        buffering incrementaux, callback de points)
  RplidarDecoder.cpp    Implémentation, aucune dépendance Arduino/matériel

test/test_keyboard_control/
  test_main.cpp        Tests unitaires (Unity) de decodeKey()/keyTimedOut(),
                        exécutés hors cible (pas besoin d'ESP32)

test/test_rplidar_decoder/
  test_main.cpp        Tests unitaires (Unity) de lib/RplidarDecoder sur des
                        paquets synthétiques (synchro, checksum, interpolation,
                        resynchronisation), hors cible

test/test_rplidar_decoder_capture/
  test_main.cpp        Test complémentaire optionnel : rejoue une capture
                        réelle du flux lidar (test/fixtures/*.bin, non
                        versionnée) ; s'auto-ignore si le fichier est absent

tools/
  keyboard_client.py    Client Python (PC) : capte w/a/s/d au clavier et les
                         envoie en boucle au robot par TCP
  capture_rplidar_serial.py  Enregistre un flux série (relais Serial2 du
                         lidar) dans un fichier binaire, pour test/fixtures/
                         (voir test/fixtures/README.md)
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
`Motor::setMotorForward/Backward/stopMotor`). `lib/RplidarDecoder` (voir plus
bas) suit le même principe pour le décodage LIDAR. Ce sont les deux seuls
modules du firmware couverts par des tests aujourd'hui ; le reste (moteurs,
pilotage LIDAR, Wi-Fi) n'est vérifiable qu'en conditions réelles, faute
d'abstraction matérielle testable.

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
checksum XOR) et démarre un scan express sur `Serial2`. Le décodage des
paquets renvoyés par le lidar est extrait dans `lib/RplidarDecoder`, sur le
même principe que `lib/KeyboardControl` : logique pure (`<cstdint>` seul,
pas d'`Arduino.h`), testable hors cible via `pio test -e native`.

Chaque paquet "capsule" (84 octets) porte un angle de départ en virgule fixe
Q6 (degrés x64) et 16 "cabines" de 2 points chacune (distance en Q2, mm x4,
+ offset d'angle en Q3, 1/8 de degré), format propriétaire Slamtec. L'angle
de chaque point n'étant fiable qu'au début du paquet, `decodeCapsulePair()`
interpole linéairement entre les angles de départ de deux paquets
consécutifs avant d'appliquer l'offset Q3 de la cabine. `StreamDecoder`
consomme le flux d'octets brut de l'UART : il se resynchronise seul sur les
demi-octets de synchro (`0xA`/`0x5`) si l'alignement est perdu (bruit, octet
manqué, redémarrage), vérifie le checksum XOR de chaque paquet, et restitue
les points valides via un callback (`ScanPoint{angle_deg, distance_mm}`).

`LidarController::poll(callback)` lit les octets disponibles sur `Serial2`
(non bloquant) et les transmet au `StreamDecoder`. `main.cpp` l'appelle à
chaque tour de `loop()`, y compris en continu pendant qu'un client de
téléop est connecté (`onLidarPoints` fait un simple `Serial.print` de debug)
— sinon le petit buffer RX matériel de l'UART déborde et le décodage
dégénère en resynchronisations/erreurs de checksum. Aucune structure de
données exploitable par le reste du firmware n'est produite pour l'instant
(pas encore de nuage de points en mémoire, pas de consommateur).

Le décodeur est testé sur des paquets synthétiques
(`test/test_rplidar_decoder`) et, en complément optionnel, sur une capture
réelle du flux série (`test/test_rplidar_decoder_capture`,
`tools/capture_rplidar_serial.py`, voir `test/fixtures/README.md`).

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

# Tests unitaires (lib/KeyboardControl, lib/RplidarDecoder), sur la machine de dev (pas d'ESP32 requis)
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
- Le décodage LIDAR (`LidarController::poll()` + `lib/RplidarDecoder`) ne
  fait encore que logger les points sur `Serial` (`onLidarPoints` dans
  `main.cpp`) ; il n'y a pas encore de consommateur de ces données (pas
  d'évitement d'obstacle, pas de SLAM).
- Du code d'encodeur odométrique (interruption `encoderISR`, comptage de
  ticks) est présent dans `main.cpp` mais commenté / non branché.
- `DriverMotor.h` utilise l'extension `.h` alors que le reste des headers
  du projet utilise `.hpp` — incohérence mineure de nommage.
