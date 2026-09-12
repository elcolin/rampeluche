# Fixtures de test (captures reelles du RPLIDAR A2M8)

`test_rplidar_decoder_capture` verifie le decodeur sur un enregistrement reel
du flux serie du lidar, en complement des paquets synthetiques de
`test_rplidar_decoder`. Ce fichier binaire n'est pas versionne (specifique a
un capteur/une session de capture) : chacun genere le sien localement.

## 1. Generer la capture

### Via l'ESP32 (cablage actuel : lidar relie uniquement a Serial2)

Remplacer temporairement le contenu de `loop()` dans `src/main.cpp` par un
simple relais brut Serial2 -> Serial :

```cpp
void loop() {
    static bool started = false;
    if (!started) {
        delay(500);
        LidCtl.startExpressScan();
        started = true;
    }
    while (Serial2.available()) {
        Serial.write(Serial2.read());
    }
}
```

Flasher (`pio run -t upload`), puis sur la machine de dev (`pip install
pyserial` si besoin) :

```
python3 tools/capture_rplidar_serial.py <port_serie_esp32> 5 test/fixtures/rplidar_a2m8_express_scan.bin
```

`<port_serie_esp32>` : le port USB de l'ESP32 (visible dans
`pio device monitor` ou le moniteur serie, ex. `/dev/tty.usbmodem14201` sur
macOS, `COM5` sur Windows). 5 secondes suffisent largement a couvrir
plusieurs tours, meme a vitesse de rotation reduite.

Remettre ensuite `loop()` dans son etat normal une fois la capture faite :
ce bloc n'est qu'un outil de capture, pas du code a garder en place.

### Via un adaptateur USB-TTL branche directement sur le lidar

Si le RPLIDAR est cable sur un adaptateur USB-TTL independant de l'ESP32, le
meme script peut lire directement le port serie du lidar. Il faut alors,
avant de lancer la capture : alimenter/piloter le moteur (ce projet utilise
une broche PWM dediee, pas de DTR automatique comme sur l'adaptateur
officiel Slamtec) et envoyer la commande Express Scan
(`0xA5 0x82 0x05 0x00 0x00 0x00 0x00 0x00 0x22`) sur ce port.

## 2. Emplacement attendu

```
test/fixtures/rplidar_a2m8_express_scan.bin
```

`test_rplidar_decoder_capture` l'ignore proprement (`IGNORE`, pas `FAIL`)
si le fichier est absent : `pio test -e native` passe sans lui, c'est un
test complementaire pour qui a le materiel sous la main.
