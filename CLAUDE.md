# CLAUDE.md

## Role

The goal is to have a functional two wheels robot with a Lidar, capable of sensor fusion and machine learning.
One of the goals is also to migrate on RTOS.

## Stack

- MCU: ESP32-S3 (`rampeluche-esp32/`), PlatformIO + Arduino framework.
- Unit tests: Unity, run off-target with `pio test -e native` (pure logic in `lib/`, no `Arduino.h`).
- Teleop client: Python (wifi keyboard control).
- Sensors: Lidar (SLAMTEC RPLidar A2M8), IMU (LSM9DS1) — see README.md for datasheets/hardware details.

## Architecture

You can read about the architecture in the README.md in the root of the project and also in rampeluche-esp32.
You can find code in rampeluche-esp32.

## Workflow

1. Write a Unity test in `test/` before touching logic (TDD).
2. Extract testable logic into `lib/<Module>/`, decoupled from `Arduino.h` when possible (see `lib/KeyboardControl`), so it runs under `pio test -e native`.
3. Implement the minimum to pass the test, then refactor.
4. Branch + PR (never commit on `main`), atomic commits.
5. If the change touches Arduino/ESP32-dependent code, also validate with `pio run -e esp32-s3-devkitc-1`.

## Agents

Custom subagents live in `.claude/agents/`:

- `code-reviewer`: read-only review of a diff/branch/PR — correctness, real-time constraints, TDD coverage, nebula naming, git workflow.
- `code-generator`: implements features/fixes following the Workflow above.

## Rules

- TDD, always write tests before coding
- Never commit on main, always branch + PR
- Make atomic commits
- Code needs to respect real time rules.
- Use nebula naming convention.
- Write comments and document when necessary.
- Keep things as concise as possible.
