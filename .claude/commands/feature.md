---
description: Pipeline complète pour une feature/fix rampeluche — implémentation (code-generator) puis revue (code-reviewer) puis doc (documentation-generator), avec arrêt si la revue demande des changements.
argument-hint: <description de la feature ou du correctif>
---

Tu orchestres le pipeline suivant pour la demande : **$ARGUMENTS**

N'implémente rien toi-même : chaque étape doit passer par le subagent dédié (outil Agent), dans cet ordre, en s'arrêtant dès qu'une étape bloque.

## 1. Implémentation

Invoque l'agent `code-generator` avec la description de la feature/du correctif ($ARGUMENTS). Rappelle-lui le workflow TDD du `CLAUDE.md` (test Unity avant le code, lib découplée d'`Arduino.h` quand possible, `pio test -e native`, puis `pio run -e esp32-s3-devkitc-1` si le changement touche du code Arduino).

Si l'agent ne peut pas terminer (ambiguïté, info hardware manquante), arrête le pipeline et demande la précision à l'utilisateur — ne passe pas à l'étape 2.

## 2. Revue

Invoque l'agent `code-reviewer` sur le diff produit à l'étape 1 (branche courante vs `main`).

- Si le verdict est **changements requis** : arrête le pipeline, restitue les findings à l'utilisateur tels quels (fichier:ligne, scénario, suggestion), et ne passe pas à l'étape 3. Ne corrige pas toi-même — repasse par `code-generator` seulement si l'utilisateur demande d'appliquer les corrections, puis relance une revue.
- Si le verdict est **OK** ou **OK avec remarques mineures** : continue à l'étape 3, et mentionne les remarques mineures dans le résumé final.

## 3. Documentation

Invoque l'agent `documentation-generator` pour mettre à jour `README.md` / sections `Stack` et `Architecture` du `CLAUDE.md` en fonction de ce qui a effectivement été implémenté (pas avant l'étape 1, jamais en avance sur le code).

## 4. Résumé

Termine par un résumé court : ce qui a été implémenté, verdict de revue, docs mises à jour, et la branche/PR à ouvrir (jamais de commit direct sur `main` — branche dédiée, commits atomiques, attribution Claude sur les commits/PR comme indiqué dans le contexte de session).
