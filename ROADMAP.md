# Roadmap Rampeluche

Document de suivi technique — état vérifié dans le code/git au 2026-09-13
(mis à jour le même jour : conflit PR #3, nettoyage `WifiHandler.hpp`
constaté fait, extraction `WifiTeleopServer`/`Motor` ajoutée aux actions).
Ne remplace pas README.md / CLAUDE.md (Stack et Architecture y restent la
source de vérité) ; ce fichier trace uniquement les jalons et leurs
dépendances.

## État réel vérifié

- `main` est à jour avec `origin/main` (dernier merge : PR #17, `/feature`
  slash command — tooling agents, hors périmètre firmware de ce document).
  Deux branches restent non mergées :
  - `feat/quadrature-encoders` : ajoute `lib/QuadratureDecoder/` + tests
    Unity (TDD respecté), `Encoder`/`EncoderController`. Pas de PR ouverte,
    juste une branche locale/remote. Ne dépend de rien d'autre, aucun
    conflit connu avec `main`.
  - PR #3 `worktree-rplidar-decoder` (**DRAFT**) : ajoute `lib/RplidarDecoder/`
    (décodeur Express Scan pur, sans `Arduino.h`, fidèle au SDK SLAMTEC),
    12 tests Unity, validation sync/checksum, `LidarController::poll()`,
    outil de capture `tools/capture_rplidar_serial.py`. **Mise à jour :**
    `gh pr view 3` renvoie désormais `mergeable: CONFLICTING` avec `main`
    — un rebase est nécessaire avant de sortir du DRAFT, en plus du
    rebranchement dans `src/main.cpp`.
- `src/main.cpp` (sur `main`) contient encore le prototype `parseExpressPacket()`
  ad-hoc (décodage non testé, pas de checksum ni resynchronisation) et une
  boucle téléop bloquante `while (client.connected())`. Le WiFi/TCP
  (`WiFiServer`, `WiFiClient`, parsing des touches) reste géré inline dans
  ce fichier, sans module dédié.
- `include/WifiHandler.hpp` (coquille vide) a été **supprimé** (PR #19,
  mergée) : la dette "header qui ne fait rien" a disparu, mais le problème
  de fond (Wi-Fi/TCP non extrait en module testable) reste entier — voir
  action court terme "extraire `WifiTeleopServer`" ci-dessous.
- `include/Motor.hpp`/`src/Motor.cpp` et `include/DriverMotor.h`/
  `src/DriverMotor.cpp` : logique PWM/direction correcte (table de vérité
  H-bridge documentée en commentaire) mais couplée à `Arduino.h` dès la
  déclaration — aucune logique pure extraite, donc aucun test Unity
  possible sur ce périmètre aujourd'hui.
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

1. **Merger `feat/quadrature-encoders` (ouvrir une PR).** Indépendant du
   reste, déjà TDD-compliant, aucun conflit connu — à sortir en premier,
   avant la PR #3, pour limiter le risque de conflits supplémentaires sur
   `main.cpp`.
2. **Rebaser la PR #3 sur `main` puis la sortir du DRAFT.** `gh pr view 3`
   montre `mergeable: CONFLICTING` — le rebase est un préalable, distinct
   du rebranchement applicatif. Une fois rebasée : remplacer le bloc
   `Serial2.available() >= 84` / `parseExpressPacket()` de `main.cpp` par
   `LidCtl.poll(onLidarPoints)` (patch documenté dans la description de la
   PR). Ferme l'issue #14.
3. **Extraire un module `lib/WifiTeleopServer/`** pour la logique
   actuellement inline dans `main.cpp` (parsing du flux TCP, gestion du
   timeout clavier) — remplace l'ancienne action "nettoyer
   `WifiHandler.hpp`" (déjà faite via PR #19, mais qui n'a fait que
   supprimer la coquille vide sans combler le vide fonctionnel). Prérequis
   structurel avant de pouvoir écrire l'action 4 sans tout mélanger dans
   `main.cpp`.
4. **Corriger la boucle téléop bloquante (issue #13).** Une fois 1-3
   mergés, remplacer `while (client.connected())` par un poll non bloquant
   dans `loop()` (solution minimale, sans RTOS) OU basculer directement sur
   deux tâches FreeRTOS (téléop / capteurs) si le jalon **M2** est engagé en
   parallèle. Nécessaire avant tout travail de fusion Lidar+IMU puisque le
   Lidar est aujourd'hui silencieusement coupé pendant la conduite.
5. **Extraire la logique PWM/direction pure de `Motor`** dans une fonction
   testable (ex. "quel `IN1`/`IN2`/duty cycle pour telle vitesse/direction
   demandée", sur le modèle de `KeyboardControl`) — aujourd'hui ce calcul
   est correct mais entièrement non testé faute d'extraction hors
   `Arduino.h`.
6. **Intégrer l'IMU LSM9DS1** (lecture brute + tests Unity sur le décodage
   des registres, sur le modèle de `lib/RplidarDecoder`). Aucun travail de
   fusion capteurs n'est possible avant cette étape : c'est un prérequis
   dur pour le jalon **M3**.

## Méthodes/approches possibles

### Structure du code (`lib/<Module>` vs découplage message-passing anticipé)

- **Option A — Un `lib/<Module>` par capteur/actionneur, `main.cpp` réduit
  à un orchestrateur mince (recommandé, prolonge le pattern déjà utilisé
  par `KeyboardControl`, `RplidarDecoder`, `QuadratureDecoder`).**
  Avantages : cohérent avec ce qui passe déjà en TDD ; chaque module reste
  testable isolément (`pio test -e native`) ; chaque module devient une
  unité naturellement encapsulable dans une tâche FreeRTOS pour M2.
  Inconvénients : demande de la discipline pour ne pas laisser la logique
  métier refuir dans `main.cpp` (dérive déjà observée avec
  `parseExpressPacket`).
- **Option B — Introduire dès maintenant des files/queues de
  message-passing entre modules, avant même M2.** Avantages : prépare le
  découplage FreeRTOS en une seule fois. Inconvénients : sur-ingénierie
  tant qu'il n'y a qu'une seule tâche (`loop()`) — complexité ajoutée sans
  bénéfice avant qu'il y ait une vraie concurrence à gérer.
- **Recommandation :** Option A maintenant (actions court terme 1-6
  ci-dessus) ; introduire les files/queues (Option B) seulement au moment
  de M2, quand plusieurs tâches FreeRTOS existeront réellement.

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
  - Merge `feat/quadrature-encoders`, rebase + sortie de DRAFT + merge PR #3
    (branchement `LidCtl.poll()` dans `main.cpp`, ferme #14).
  - Extraction `lib/WifiTeleopServer/` (remplace l'ancienne action
    "nettoyer `WifiHandler.hpp`", déjà faite via PR #19).
  - Fix issue #13 (poll non bloquant ou premier découplage FreeRTOS minimal).
  - Extraction de la logique PWM/direction pure de `Motor` (testable Unity).
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
