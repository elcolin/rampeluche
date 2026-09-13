# CLAUDE.md

## Role

The goal is to have a functional two wheels robot with a Lidar, capable of sensor fusion and machine learning.
One of the goals is also to migrate on RTOS.

## Stack

- MCU: ESP32-S3 (`rampeluche-esp32/`), PlatformIO + Arduino framework.
- Unit tests: Unity, run off-target with `pio test -e native` (pure logic in `lib/`, no `Arduino.h`).
- Teleop: Python client (`tools/keyboard_client.py`) sending w/a/s/d over Wi-Fi — ESP32 SoftAP + raw TCP socket. Session logic (key decoding, safety-cutoff timeout latch) lives in `lib/WifiTeleopServer`, decoupled from Arduino.h; `main.cpp` only does the low-level socket read loop.
- Motor control: differential drive through an H-bridge driver (TB6612-style), speed as a 0-100% PWM duty cycle.
- Sensors: Lidar (SLAMTEC RPLidar A2M8, wired; request packets built by `lib/RplidarProtocol` and Express Scan packets decoded by `lib/RplidarDecoder` — sync/checksum + angle-distance interpolation between consecutive capsule packets — both pure logic, unit-tested off-target, not yet consumed for navigation), IMU (LSM9DS1, not yet integrated) — see README.md for datasheets/hardware details.

## Architecture

Main flow: Wi-Fi keyboard client -> SoftAP + raw TCP -> `WifiTeleopServer` (decodes each key into a left/right motor command via `KeyboardControl`, latches the safety-stop on client timeout) -> H-bridge differential drive. The blocking per-client read loop in `main.cpp` is unchanged by this. Lidar is wired and its frames are decoded, but not yet consumed (no obstacle avoidance/SLAM yet).

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
- `tech-architect`: read-only analysis of project state (code, git log, issues) against the goals above — proposes short-term next steps, alternative technical approaches (e.g. sensor fusion, RTOS migration strategy) with trade-offs and a recommendation, and a milestone roadmap; only writes a file if explicitly asked to persist a roadmap doc.

## Rules

- Before tackling any request, check `.claude/agents/` for a subagent that matches the task and use it instead of handling it ad hoc.
- TDD, always write tests before coding
- Never commit on main, always branch + PR
- Make atomic commits
- Code needs to respect real time rules.
- Use nebula naming convention.
- Write comments and document when necessary.
- Keep things as concise as possible.
- Every commit and PR an agent creates must carry Claude's attribution (commit trailer + PR footer), using whatever exact wording the running session provides — never fabricate one if none is given. When the work is done by a named subagent (e.g. `tech-architect`, `code-generator`, `documentation-generator`, `code-reviewer`, `agent-generator`), add that agent's name to the signature without altering the session-provided wording:
  - Commit trailer: insert `(<agent-name>)` right after the model name, e.g. `Co-Authored-By: Claude Sonnet 5 (tech-architect) <noreply@anthropic.com>`.
  - PR footer: append `via the \`<agent-name>\` agent` after the session-provided line, e.g. `🤖 Generated with [Claude Code](https://claude.com/claude-code) via the \`tech-architect\` agent`.
  When the work is done directly by the main session (no named subagent involved), leave the trailer/footer unchanged as provided by the session.
- Never include a Claude session URL/link (e.g. a `Claude-Session:` trailer or any `https://claude.ai/code/session_...` link) in a commit message or PR description on this repo — it is public, and such a link must be treated as a potential secret. Only the Co-Authored-By trailer (and PR footer) attribution is required.
- Before creating or updating a PR, an agent estimates its own token usage for the branch/PR's unit of work by summing `input_tokens`, `output_tokens`, `cache_creation_input_tokens`, and `cache_read_input_tokens` across the assistant turns in its own transcript (locally, from `~/.claude/projects/<project>/<sessionId>/subagents/agent-<id>.jsonl`). If other agents contributed to the same branch/PR (e.g. a `/feature` pipeline: code-generator → code-reviewer → documentation-generator), find their sibling transcripts in the same `subagents/` folder via `agentType` in each `agent-<id>.meta.json` and sum their estimates in too. Add one line to the PR footer, right after the `🤖 Generated with...` line, e.g.:
  `Tokens estimés (local, approximatif) : code-generator ~45k, code-reviewer ~28k`
  This is a local, approximate estimate — never an official billing figure — and it must only ever contain agent names and aggregated numbers: never a file path, session ID, or UUID. If an agent's own usage data is missing or unreadable, it omits the line entirely rather than inventing a number.
