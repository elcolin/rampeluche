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
        |-- lit un octet sur le socket client -> WifiTeleopServer::onKeyReceived()
        |     `-- delegue a KeyboardControl::decodeKey() -> DriveCommand (gauche/droite)
        |-- pas d'octet dispo -> WifiTeleopServer::checkTimeout() (delegue a keyTimedOut())
        |-- applyDriveCommand()/applyMotorCommand() -> DriverMotor -> Motor -> pins PWM/IN1/IN2
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
  LidarController.hpp Pilotage du RPLidar sur Serial2 (setup, scan express,
                       poll()) ; delegue l'encodage des requetes a
                       lib/RplidarProtocol et le decodage des trames a
                       lib/RplidarDecoder

src/                 Implémentation, dépend d'Arduino.h / matériel réel
  main.cpp            setup()/loop() : Wi-Fi, serveur TCP (lecture bas niveau du
                       socket uniquement), boucle de pilotage, poll() du LIDAR
                       (LidarController -> RplidarDecoder)
  DriverMotor.cpp
  Motor.cpp
  LidarController.cpp

lib/KeyboardControl/  Logique pure de décision "touche -> commande moteur"
  KeyboardControl.hpp   Types (MotorAction, MotorCommand, DriveCommand),
                         constantes de vitesse/timeout, decodeKey(), keyTimedOut()
  KeyboardControl.cpp   Implémentation, aucune dépendance Arduino/matériel

lib/WifiTeleopServer/ Logique pure de session téléop (au-dessus de KeyboardControl)
  WifiTeleopServer.hpp  Classe WifiTeleopServer : onKeyReceived() (touche -> DriveCommand,
                         reset du timeout), checkTimeout() (latch du stop de sécurité)
  WifiTeleopServer.cpp  Implémentation, aucune dépendance Arduino/matériel

lib/RplidarDecoder/   Logique pure de décodage des paquets "Express Scan"
                      du RPLIDAR A2M8 (synchro, checksum, interpolation
                      angle/distance, resynchronisation sur un flux d'octets)
  RplidarDecoder.hpp    Types (ScanPoint), fonctions de décodage d'un paquet
                        (decodeCapsulePair()...), StreamDecoder (etat +
                        buffering incrementaux, callback de points)
  RplidarDecoder.cpp    Implémentation, aucune dépendance Arduino/matériel

lib/RplidarProtocol/  Logique pure d'encodage des paquets de requête envoyés
                      au RPLIDAR (en-tête, commande, payload, checksum XOR)
  RplidarProtocol.hpp   buildRequestPacket(), requestPacketSize()
  RplidarProtocol.cpp   Implémentation, aucune dépendance Arduino/matériel

test/test_keyboard_control/
  test_main.cpp        Tests unitaires (Unity) de decodeKey()/keyTimedOut(),
                        exécutés hors cible (pas besoin d'ESP32)

test/test_wifi_teleop_server/
  test_main.cpp        Tests unitaires (Unity) de WifiTeleopServer (décodage touche,
                        déclenchement/latch du timeout), exécutés hors cible

test/test_rplidar_decoder/
  test_main.cpp        Tests unitaires (Unity) de lib/RplidarDecoder sur des
                        paquets synthétiques (synchro, checksum, interpolation,
                        resynchronisation), hors cible

test/test_rplidar_protocol/
  test_main.cpp        Tests unitaires (Unity) de lib/RplidarProtocol :
                        encodage de requête (avec/sans payload), checksum,
                        dimensionnement de buffer via requestPacketSize()

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

`lib/WifiTeleopServer` regroupe la logique d'une session téléop au-dessus de
`KeyboardControl` : la classe `WifiTeleopServer` ne dépend elle non plus que
de `KeyboardControl.hpp`, pas d'`Arduino.h`/`WiFiClient`. `onKeyReceived()`
délègue le décodage à `decodeKey()` et réarme le timeout ; `checkTimeout()`
délègue à `keyTimedOut()` et ne remonte l'ordre de stop qu'une seule fois par
période de silence (`TimeoutResult::triggered`). Testable elle aussi sous
`pio test -e native` (voir `test/test_wifi_teleop_server`).

`src/main.cpp` reste le seul endroit qui connecte cette logique au matériel
réel : la boucle `while (client.connected())` lit les octets bruts du socket
TCP et les pousse à `WifiTeleopServer::onKeyReceived()` (ou appelle
`checkTimeout()` quand rien n'est disponible), puis `applyDriveCommand()`/
`applyMotorCommand()` traduisent le `DriveCommand`/`MotorCommand` obtenu en
appels `Motor::setMotorForward/Backward/stopMotor`. `lib/RplidarDecoder` et
`lib/RplidarProtocol` (voir plus bas) suivent le même principe pour le
protocole LIDAR (décodage des trames reçues / encodage des requêtes
envoyées), via `LidarController::poll()`/`startExpressScan()`. Ce sont les
quatre modules du firmware couverts par des tests aujourd'hui
(`KeyboardControl`, `WifiTeleopServer`, `RplidarDecoder`, `RplidarProtocol`) ;
le reste (moteurs, pilotage LIDAR bas niveau, Wi-Fi bas niveau) n'est
vérifiable qu'en conditions réelles, faute d'abstraction matérielle
testable.

### Coupure de sécurité si le client décroche

`WifiTeleopServer` mémorise l'horodatage de la dernière touche reçue. Si
aucune touche n'arrive pendant `KEY_TIMEOUT_MS` (500 ms, défini dans
`KeyboardControl.hpp`), `checkTimeout()` retourne une commande d'arrêt pour
les deux moteurs, que `main.cpp` applique via `applyDriveCommand()`. Ça
protège contre une perte de connexion Wi-Fi ou un client qui plante avec le
robot lancé. Côté client Python, l'envoi est cadencé à 150 ms
(`SEND_INTERVAL`), volontairement bien en dessous du timeout firmware, pour
qu'un simple relâchement de touche coupe les moteurs de façon réactive sans
attendre le timeout de sécurité.

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

### LIDAR : protocole RPLidar "express scan"

`LidarController` orchestre le pilotage du RPLidar sur `Serial2` (`setup()`,
`startExpressScan()`, `poll()`), mais délègue le protocole série à deux libs
pures (`<cstdint>` seul, pas d'`Arduino.h`), testables hors cible via
`pio test -e native`, sur le même principe que `lib/KeyboardControl` :

- `lib/RplidarProtocol` construit les paquets de requête envoyés au lidar
  (`buildRequestPacket()` : en-tête `0xA5` + commande + taille payload +
  payload + checksum XOR). `requestPacketSize(payloadLen)` calcule la taille
  exacte de buffer requise (overhead 4 octets + payload) ; `startExpressScan()`
  dimensionne son buffer avec ce helper plutôt qu'un magic number, pour
  éviter un débordement si la taille du payload change.
- `lib/RplidarDecoder` décode les paquets renvoyés par le lidar (voir
  ci-dessous).

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
L'encodage des requêtes (`lib/RplidarProtocol`) est testé séparément
(`test/test_rplidar_protocol`) sur des vecteurs de référence issus de la
doc protocole RPLIDAR.

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

# Tests unitaires (lib/KeyboardControl, lib/WifiTeleopServer,
# lib/RplidarDecoder, lib/RplidarProtocol), sur la machine de dev (pas d'ESP32 requis)
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

- Le décodage LIDAR (`LidarController::poll()` + `lib/RplidarDecoder`) ne
  fait encore que logger les points sur `Serial` (`onLidarPoints` dans
  `main.cpp`) ; il n'y a pas encore de consommateur de ces données (pas
  d'évitement d'obstacle, pas de SLAM).
- Du code d'encodeur odométrique (interruption `encoderISR`, comptage de
  ticks) est présent dans `main.cpp` mais commenté / non branché.
- `LidarController::startExpressScan()` bloque ~500 ms (`delay()`) le temps
  que le moteur du lidar atteigne sa vitesse nominale. Appelé une seule fois
  au démarrage (premier tour de `loop()`), donc sans impact récurrent
  aujourd'hui, mais à traiter avant toute boucle temps réel plus stricte.
- La boucle `while (client.connected())` de `main.cpp` reste bloquante (elle
  ne rend la main qu'à la déconnexion du client) ; l'extraction de
  `WifiTeleopServer` n'a pas touché à ce point.
- `DriverMotor.h` utilise l'extension `.h` alors que le reste des headers
  du projet utilise `.hpp` — incohérence mineure de nommage.
