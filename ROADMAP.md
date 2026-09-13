# Roadmap Rampeluche

Document de suivi technique — état vérifié dans le code/git au 2026-09-13.
Ne remplace pas README.md / CLAUDE.md (Stack et Architecture y restent la
source de vérité) ; ce fichier trace uniquement les jalons et leurs
dépendances.

## État réel vérifié

- `main` est à jour avec `origin/main`. Deux branches ne sont pas mergées :
  - `feat/quadrature-encoders` : ajoute `lib/QuadratureDecoder/` + tests
    Unity (TDD respecté), `Encoder`/`EncoderController`. Prête à être
    rebasée/mergée, ne dépend de rien d'autre.
  - PR #3 `worktree-rplidar-decoder` (**DRAFT**) : ajoute `lib/RplidarDecoder/`
    (décodeur Express Scan pur, sans `Arduino.h`, fidèle au SDK SLAMTEC),
    12 tests Unity, validation sync/checksum, `LidarController::poll()`,
    outil de capture `tools/capture_rplidar_serial.py`. C'est un travail
    quasi complet : il ne manque que le rebranchement dans `src/main.cpp`
    (le worktree ne voyait pas les changements WiFi/téléop faits en
    parallèle sur `main`).
- `src/main.cpp` (sur `main`) contient encore le prototype `parseExpressPacket()`
  ad-hoc (décodage non testé, pas de checksum ni resynchronisation) et une
  boucle téléop bloquante `while (client.connected())`.
- `include/WifiHandler.hpp` existe mais est une coquille vide (`WifiHandler`
  sans membre) : le WiFi/TCP est en réalité géré directement dans `main.cpp`
  (`WiFiServer`, `WiFiClient`), pas dans cette classe.
- Torque moteur (issue #1, "Configure esp32 signal for torque reduction") :
  **fermée**. `Motor::stopMotor()` met `IN1=LOW, IN2=LOW` ce qui correspond
  bien au mode "Stop" (roue libre) du driver, pas au mode "Short brake" — le
  fix attendu a bien été implémenté. Aucune incohérence de code résiduelle
  détectée sur ce point ; à re-vérifier physiquement si le symptôme
  (moteur qui creuse le plastique) réapparaît.
- IMU LSM9DS1 : aucune trace dans `lib/`, `src/`, `include/` — pas encore
  câblée ni codée. La fusion capteurs (Lidar + IMU) ne peut pas démarrer
  avant son intégration.
- Migration RTOS : le core Arduino-ESP32 embarque déjà FreeRTOS (`loop()`
  tourne dans une tâche FreeRTOS), mais le firmware ne l'utilise pas
  explicitement (une seule boucle séquentielle) — voir issue #13 ci-dessous
  pour la conséquence concrète de ce choix.

### Dette identifiée par le `code-reviewer` (déjà en issues GitHub)

- **Issue #13** — `main.cpp` : la boucle `while (client.connected())`
  bloque tout le reste de `loop()` (dont la lecture `Serial2` du Lidar) tant
  qu'un client téléop reste connecté ; un `delay(500)` supplémentaire
  retarde encore la reprise après déconnexion. Concrètement : dès qu'on
  pilote le robot, le flux Lidar est perdu/désynchronisé. C'est la preuve
  par le code que le découplage FreeRTOS (jalon **M2**) n'est pas un
  exercice théorique mais un prérequis pour que téléop et lecture Lidar
  coexistent.
- **Issue #14** — Logique de protocole RPLidar (`parseExpressPacket`,
  `compute_checksum`, `build_request` dans `main.cpp`/`LidarController.cpp`)
  non extraite dans une lib testable, sans tests Unity, sans validation de
  checksum/sync (désync silencieuse possible). **La PR #3 traite déjà ce
  point** (extraction + 12 tests + validation checksum/sync) ; le travail
  restant est de sortir la PR du mode DRAFT, la rebrancher dans `main.cpp`
  et fermer l'issue #14 à la fusion — pas de le refaire dans un jalon
  ultérieur.

## Prochaines étapes court terme

1. **Sortir la PR #3 du DRAFT et la brancher dans `main.cpp`.** Le décodeur
   testé (`lib/RplidarDecoder`) existe déjà ; le laisser en draft pendant
   que `main.cpp` continue d'évoluer avec le prototype non testé augmente
   le risque de divergence/perte du travail. Remplacer le bloc
   `Serial2.available() >= 84` / `parseExpressPacket()` par
   `LidCtl.poll(onLidarPoints)` (patch déjà documenté dans la description
   de la PR). Ferme l'issue #14.
2. **Merger `feat/quadrature-encoders`.** Indépendant du reste, déjà
   TDD-compliant, ne bloque aucun autre chantier — à sortir en premier pour
   limiter les conflits de rebase avec la PR #3 (les deux touchent
   `main.cpp`).
3. **Corriger la boucle téléop bloquante (issue #13).** Une fois #1 et #2
   mergés, remplacer `while (client.connected())` par un poll non bloquant
   dans `loop()` (solution minimale, sans RTOS) OU basculer directement sur
   deux tâches FreeRTOS (téléop / capteurs) si le jalon **M2** est engagé en
   parallèle. Nécessaire avant tout travail de fusion Lidar+IMU puisque le
   Lidar est aujourd'hui silencieusement coupé pendant la conduite.
4. **Nettoyer `include/WifiHandler.hpp`.** Soit y déplacer la logique
   `WiFiServer`/`WiFiClient` actuellement dans `main.cpp` (cohérent avec la
   décomposition déjà appliquée à `DriverMotor`/`LidarController`), soit
   supprimer le fichier s'il ne sera pas utilisé — un header vide qui
   n'implémente rien de ce que son nom promet est une dette silencieuse.
5. **Intégrer l'IMU LSM9DS1** (lecture brute + tests Unity sur le décodage
   des registres, sur le modèle de `lib/RplidarDecoder`). Aucun travail de
   fusion capteurs n'est possible avant cette étape : c'est un prérequis
   dur pour le jalon **M3**.

## Méthodes/approches possibles

### RTOS : FreeRTOS (déjà présent via Arduino-ESP32) vs bare-metal

- **Option A — Tâches FreeRTOS explicites (recommandé).** Le core
  Arduino-ESP32 tourne déjà sur FreeRTOS ; il suffit de créer des tâches
  dédiées (`xTaskCreatePinnedToCore`) pour téléop, lecture Lidar,
  lecture IMU, contrôle moteur, avec des files (`xQueueSend`/`Receive`)
  pour le découplage. Avantages : résout directement l'issue #13 sans
  réécriture du framework, exploite les deux cœurs du S3, API bien
  documentée. Inconvénients : discipline requise sur les priorités/tailles
  de pile, risque de race conditions sur les ressources partagées
  (`Serial2`, moteurs) si mal cloisonné.
- **Option B — Bare-metal / boucle unique optimisée.** Réécrire `loop()`
  en state machine non bloquante (polling avec timeouts courts partout).
  Avantages : pas de nouvelle dépendance, contrôle total du timing.
  Inconvénients : ne s'attaque pas au fond du problème (un seul thread
  d'exécution) — dès qu'on ajoute IMU + SLAM + téléop + moteurs, la
  complexité de la state machine explose et redevient fragile, ce qui va
  à l'encontre de l'objectif explicite de migration RTOS du CLAUDE.md.
- **Recommandation :** Option A. Le coût de migration est faible (le
  runtime est déjà là), elle répond concrètement à l'issue #13, et elle
  seule tient dans la durée face à l'ajout de l'IMU + SLAM.

### Fusion Lidar + IMU

- **Option A — Complementary/Madgwick filter (orientation IMU) +
  recalage Lidar en aval (scan matching léger, type ICP 2D simplifié).**
  Avantages : coût CPU compatible ESP32-S3 (pas de FPU dédiée mais
  cœurs à 240 MHz suffisants pour un filtre léger), latence prévisible,
  bibliothèques existantes (ex. adaptations de Madgwick/Mahony en C).
  Inconvénients : dérive à long terme si pas de correction extérieure,
  scan matching simplifié moins robuste qu'un SLAM complet en environnement
  dynamique.
- **Option B — EKF (Extended Kalman Filter) fusionnant IMU + odométrie
  encodeurs + recalage Lidar, façon robot_localization/ROS mais réimplémenté
  en C léger.** Avantages : fusion statistiquement correcte, exploite les
  encodeurs déjà présents (`feat/quadrature-encoders`), extensible vers du
  SLAM (ex. gmapping-like) plus tard. Inconvénients : plus complexe à
  implémenter/tester en TDD pur C++ embarqué, coût CPU/mémoire plus élevé
  (matrices, inversions), à valider par profiling sur cible avant de
  s'engager.
- **Recommandation :** commencer par l'Option A (complementary filter +
  odométrie encodeurs) comme brique M3, car les encodeurs et le décodeur
  Lidar existent déjà quasiment testés — livrable rapide et vérifiable en
  Unity. Migrer vers l'EKF (Option B) seulement quand un besoin concret de
  précision de localisation apparaît (jalon SLAM, M5+), pas avant.

## Roadmap par jalons

- **M0 — Consolidation de l'existant (prérequis à tout le reste)**
  - Merge `feat/quadrature-encoders`, sortie de DRAFT + merge PR #3
    (branchement `LidCtl.poll()` dans `main.cpp`, ferme #14).
  - Fix issue #13 (poll non bloquant ou premier découplage FreeRTOS minimal).
  - Nettoyage `WifiHandler.hpp`.
  - Dépendances : aucune — c'est le socle de tout ce qui suit.

- **M1 — Lidar exploité en continu**
  - Consommer réellement les `ScanPoint` produits par `LidCtl.poll()`
    (aujourd'hui juste affichés en `Serial.print` dans l'exemple de la
    PR #3) : structure de données d'un scan complet, exposition vers le
    reste du firmware.
  - Dépend de M0 (décodeur branché + boucle non bloquante, sinon le flux
    reste coupé pendant la téléop).

- **M2 — Découplage FreeRTOS (tâches téléop / Lidar / moteurs)**
  - Implémenter l'option A ci-dessus (tâches + files) si le fix minimal de
    M0 pour #13 n'est qu'un pansement (poll) plutôt qu'une vraie séparation.
  - Dépend de M0 ; conditionne la suite car IMU + fusion + SLAM ajoutent
    des flux concurrents supplémentaires qu'une boucle unique ne pourra
    plus absorber.

- **M3 — Intégration IMU LSM9DS1 + odométrie**
  - Driver I2C/SPI + décodage registres testé en Unity (sur le modèle
    `lib/RplidarDecoder`), fusion basique avec les encodeurs
    (`feat/quadrature-encoders`) pour une estimation de pose odométrique.
  - Dépend de M2 (une tâche IMU dédiée a besoin du découplage RTOS pour ne
    pas être bloquée par la téléop, même problème que le Lidar aujourd'hui).

- **M4 — Fusion Lidar + IMU (complementary filter, Option A)**
  - Recalage des scans Lidar avec l'orientation/déplacement estimés par
    IMU+encodeurs (M3).
  - Dépend de M1 (scans exploitables) et M3 (odométrie disponible).

- **M5 — Cartographie / SLAM léger**
  - Occupancy grid ou scan-matching incrémental, adapté aux ressources
    ESP32-S3 (pas de SLAM complet type Cartographer sur ce MCU — à évaluer :
    exécution locale minimale vs déport du calcul lourd sur un hôte via
    WiFi, à trancher une fois M4 mesuré en pratique).
  - Dépend de M4.

- **M6 — Navigation autonome de base (évitement d'obstacles)**
  - Boucle de contrôle réactive sur la carte/scan fusionné, sans ML.
  - Dépend de M5 (ou au minimum de M4 si une navigation réactive pure
    Lidar+IMU suffit avant d'avoir une carte).

- **M7 — Machine learning**
  - Une fois qu'un pipeline de perception fiable et testé existe
    (M4-M6), envisager un modèle embarqué (classification d'obstacles,
    apprentissage de trajectoire) — prématuré tant que la donnée capteur
    fusionnée n'est pas stable et vérifiée par tests.
  - Dépend de M4 au minimum (données de fusion propres), idéalement M5/M6.
