# CLAUDE.md

## Role

The goal is to have a functional two wheels robot with a Lidar, capable of sensor fusion and machine learning.
One of the goals is also to migrate on RTOS.

## Stack

- MCU: ESP32-S3 (`rampeluche-esp32/`), PlatformIO + Arduino framework.
- Unit tests: Unity, run off-target with `pio test -e native` (pure logic in `lib/`, no `Arduino.h`).
- Teleop: Python client (`tools/keyboard_client.py`) sending w/a/s/d over Wi-Fi — ESP32 SoftAP + raw TCP socket, motor safety cutoff on client timeout.
- Motor control: differential drive through an H-bridge driver (TB6612-style), speed as a 0-100% PWM duty cycle.
- Sensors: Lidar (SLAMTEC RPLidar A2M8, wired and its frames decoded, not yet consumed for navigation), IMU (LSM9DS1, not yet integrated) — see README.md for datasheets/hardware details.

## Architecture

Main flow: Wi-Fi keyboard client -> SoftAP + raw TCP -> decoded into a left/right motor command -> H-bridge differential drive. Lidar is wired and its frames are decoded, but not yet consumed (no obstacle avoidance/SLAM yet).

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

- `code-reviewer`: read-only review of a diff/branch/PR — correctness, real-time constraints, TDD coverage, nebula naming, git workflow; files a GitHub issue for significant problems outside the diff's scope.
- `code-generator`: implements features/fixes following the Workflow above.
- `documentation-generator`: keeps README.md and the Stack/Architecture sections here up to date from the actual code/hardware.
- `agent-generator`: creates/updates/cleans up subagent definitions in `.claude/agents/`, keeps this section in sync, and always leaves `model: inherit` (no extra billing) — never pins `sonnet`/`opus`, only `haiku` to cut cost on low-risk mechanical agents.

## Rules

- Before tackling any request, check `.claude/agents/` for a subagent that matches the task and use it instead of handling it ad hoc.
- TDD, always write tests before coding
- Never commit on main, always branch + PR
- Make atomic commits
- Code needs to respect real time rules.
- Use nebula naming convention.
- Write comments and document when necessary.
- Keep things as concise as possible.
- Every commit and PR an agent creates must carry Claude's attribution (commit trailer + PR footer), using whatever exact wording the running session provides — never fabricate one if none is given.
