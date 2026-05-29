# Game-Mechanics Verification Harness — Implementation Plan

> Self-reviewed in lieu of user design-approval (user is non-technical on this and would
> blind-approve). Goal: tooling that lets **Claude Code** develop + verify game mechanics
> autonomously, on a *faithful* foundation, with verdicts that are *calibrated* so they
> can be trusted. User judges only "are we there / not."

**Goal:** Run game scenarios in a REAL game loop, emit machine- + vision-readable
observations, and prove the harness is trustworthy by calibrating it against known-good
and known-bad before using it to judge anything.

**Architecture:** A runtime `UScenarioController` (`UGameInstanceSubsystem`, PoF module —
NOT editor-only) executes a scenario JSON inside a naturally-ticked PIE or a standalone
`-game` session (never manual `World::Tick`). It drives real input, samples time-based
observations (live pose, transform, velocity, extensible game-state) + captures a frame
sequence, and writes `observations.json` + `frame_NN.png` + `DONE`. Claude launches it
(background, PID-tracked, watchdog'd), reads a tiled contact-sheet PNG + the JSON, judges.

**Trust anchor:** CALIBRATION-FIRST. Known-good (MoverExamples native Manny walking) and
known-bad (anim-less T-pose pawn). No verdict is trusted until the harness separates them
on BOTH the pose metric and the frames.

**Tech stack:** UE 5.7 C++ runtime subsystem; EnhancedInput (key + action injection);
SceneCapture2D→render target→PNG; JSON via FJsonSerializer; PoF Bridge (`/pof/python/run`)
for the PIE-natural launch; Python reader + PIL/none for contact-sheet tiling; Claude
reads PNG/JSON.

---

## Why the prior approach failed (the constraint this plan obeys)

`RunScenarioEx` (PoFEditor) ticked PIE via `World::Tick` from editor automation. Result:
simulated keys moved nothing; action injection moved the pawn once then never; bone reads
returned the REFERENCE pose; spawn varied run-to-run. Root cause = editor-automation tick
≠ real game loop. This plan runs verification INSIDE a real game loop and never trusts an
uncalibrated metric.

---

## Phase 0 — Calibration target + reader (build the trust anchor FIRST)

### Task 0.1: Known-bad pawn + known-good reference scenarios
- Create `/Game/Tests/BP_TPosePawn` — a Character with SkeletalMesh = SKM_Manny and NO
  anim class (guaranteed reference/T-pose). Create test map `/Game/Maps/TestHarness` (lit:
  DirectionalLight + SkyLight + floor; reuse the arena lighting that renders cleanly).
- Identify the MoverExamples demo map where the native Manny walks (known-good). Confirm it
  loads + possesses.
- Artifact: two scenario JSONs — `calib_good.json`, `calib_bad.json`.

### Task 0.2: Contact-sheet reader (Claude-side)
- `Content/Python/observation/contact_sheet.py`: read an artifact dir's `frame_*.png`, tile
  into one `sheet.png` (grid), and print a one-line summary of `observations.json`
  (per-checkpoint: t, loc, speed, armDroopL/R, verdict). Pure stdlib if possible (manual
  PNG tiling is hard without PIL — fallback: emit an HTML/montage or just have Claude read
  the N frames directly; decide at build time).
- Test: run on a hand-made dir of 2 PNGs → produces sheet + summary.

---

## Phase 1 — ScenarioController (runtime, real game loop)

### Task 1.1: Scenario spec + observation structs (runtime module PoF)
- `FScenarioInputEvent { FString Key; FString ActionPath; FVector2D Value; float Start; float Duration; }`
- `FScenarioCheckpoint { float Time; bool CaptureFrame; }`
- `FScenarioObservation { float Time; FVector Location; float Speed; float ArmDroopL/R;
  bool PoseValid; FString Frame; TMap<FString,float> GameState; }`
- Parse from JSON file path; serialize observations to JSON.

### Task 1.2: `UScenarioController` core (UGameInstanceSubsystem, PoF module)
- On `Initialize`, read scenario path from cmdline `-PoFScenario=<path>` OR a fixed inbox
  file; if none, stay dormant (no-op in normal play).
- On the first tick where a possessed pawn exists + a settle delay elapsed, START: drive the
  input timeline (key-level via the local player's input / PlayerController InputKey in the
  REAL loop; action-level via EnhancedInput subsystem), keyed off `GetTimeSeconds()`.
- At each checkpoint time: `RefreshBoneTransforms` then read upperarm/lowerarm component-space
  → droop; read location/velocity; capture a frame (SceneCapture2D→RT→PNG, framed on pawn);
  append observation.
- On completion: write `observations.json` + `DONE`; if launched standalone, `RequestExit`.
- Determinism: time-based throughout; tolerate variable frame dt.

### Task 1.3: Compile + register (one rebuild)
- Add to PoF.Build.cs deps as needed (EnhancedInput, RenderCore, Json). Build PoFEditor
  (editor binary runs the runtime module). Verify the subsystem is discoverable.

---

## Phase 2 — Launch paths

### Task 2.1: PIE-natural launch (bridge verb) — primary
- Bridge/Python verb: write the scenario JSON to the inbox, arm the controller, call
  `RequestPlaySession`, RETURN (do NOT manual-tick). Editor ticks PIE naturally.
- Claude polls for `DONE` (background/poll), then reads artifacts. End PIE after.

### Task 2.2: Standalone `-game` launch — isolation fast-follow
- Launch `UnrealEditor.exe PoF.uproject <Map> -game -PoFScenario=<json>
  -UseFixedTimeStep -FixedTimeStep=0.01667 -windowed -ResX=640 -ResY=360 -abslog=<log>`
  as a background, PID-tracked process with a watchdog timeout (kill ONLY that PID on
  timeout). On exit, read artifacts.

---

## Phase 3 — CALIBRATION GATE (must pass before any feature verdict)

### Task 3.1: Run calib_good + calib_bad through the harness
- Known-good MUST report: pawn translates under input, armDroop high (arms down) OR frames
  clearly show locomotion; verdict=WALKS.
- Known-bad MUST report: no/!= locomotion, armDroop ~0 / frames clearly T-pose; verdict=TPOSE.
- If the harness CANNOT separate them → STOP. The foundation (likely the launch mode) is
  still unfaithful; switch PIE-natural↔standalone, or fix, until separation is clean.
- Record the calibrated thresholds (droop cutoff, displacement cutoff) from the two anchors.

---

## Phase 4 — Apply to the real feature (only after Phase 3 passes)

### Task 4.1: Verify the IMC fix with real keys
- Scenario: real key W/A/S/D each → assert distinct, correct displacement directions.

### Task 4.2: Diagnose + fix the player T-pose
- Run the player on TestHarness; read calibrated verdict + frames. If T-pose, rebase on the
  native-Manny anim approach (ABP_Manny / BS_MM_WalkRunStrafe driven by our Speed/Direction,
  or fix BS_Locomotion to include an idle sample), re-verify against the calibrated harness.

### Task 4.3: Generalize an observation probe (proof of breadth)
- Add ONE game-state observation (e.g., ability-activated or damage-dealt) to prove the
  scenario/observation model extends beyond locomotion → this is the bridge to using the
  harness for arbitrary mechanics.

---

## Risks / open questions (tracked, not hidden)
- **Is naturally-ticked PIE faithful?** HYPOTHESIS — Phase 3 calibration proves or refutes it.
  If refuted, standalone `-game` is the fallback (Task 2.2 already specced).
- **Frame capture in standalone windowed** — SceneCapture2D→RT→PNG renders independent of the
  swapchain; proven to work on a lit scene. Keep the lit TestHarness map.
- **Bone read faithfulness** — must be validated by calibration (known-bad must read ~0 droop
  AND look T-posed; if the metric still reads ref-pose in a real loop, drop it and rely on
  frames — the frames are the ultimate oracle either way).
- **Contact-sheet tiling without PIL** — fallback to reading N frames directly.
