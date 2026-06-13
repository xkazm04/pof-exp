"""Verification gate — apply-then-PROVE, so no change lands on a guess.

Runs a named gate's scenario through the live harness, then judges the result against the
last-known-good on THREE independent axes:
  1. NUMERIC   — trace/obs metric contracts (reused from run_contracts) + baseline regression
  2. VISUAL    — golden-image diff of the key frame (image_diff) vs the saved golden
  3. SEMANTIC  — a separate vision-judge (an LLM pass on the key frame vs the gate's intent),
                 run by the ORCHESTRATOR (not here): this script emits key_frame + intent for it.

Verdict: any numeric FAIL/REGRESSED or visual regression => REJECT. First run with no golden
ESTABLISHES it (warns, like UE's "Add New"). --promote saves the new last-known-good
(baseline pass-map + key-frame golden) only when numeric+visual are clean.

Usage: python verify_gate.py <gate> [--map M] [--promote]
"""
import json
import os
import shutil
import subprocess
import sys

CLAUDE = (shutil.which("claude.exe")
          or r"C:\Users\kazda\AppData\Roaming\npm\node_modules\@anthropic-ai\claude-code\bin\claude.exe")

import image_diff
from run_contracts import metrics, run_scenario  # same dir; reuse the harness + metric layer

OBS_W = r"C:\Users\kazda\AppData\Local\Temp\pof_obs"
GOLD_DIR = os.path.join(OBS_W, "golden")
GATE_BASELINE = os.path.join(OBS_W, "gate_baseline.json")
VERDICT = os.path.join(OBS_W, "gate_verdict.json")

# Each gate = scenario + numeric checks + the key frame to diff/judge + the human intent the
# vision-judge scores against. Roll uses the open -Y arena lane + real-viewport mid-roll frame.
GATES = {
    "roll-forward": {
        "map": "/Game/Maps/VerticalSlice",
        "intent": ("A forward dodge-roll along the red aim arrow: the character tucks and rolls, "
                   "stays in contact with the floor without clipping below it or floating above it, "
                   "travels a body-length or two, and recovers on its feet."),
        # DETERMINISTIC capture (proven <=2.1% run-to-run at full frame vs 2.7-46% wall-clock):
        # enemy destroyed at runtime, captures keyed to MONTAGE POSITION, fixed world camera.
        "scenario": {"inputs": [{"event": "set_cursor", "event_arg": "15,-600,90", "start": 0.2, "duration": 2.2},
                                {"key": "SpaceBar", "start": 1.0, "duration": 0.1}],
                     "total_seconds": 2.6, "num_samples": 4, "settle": 0.6,
                     "remove_actor_classes": ["BP_VSEnemy_C"],
                     "det_capture_positions": [0.05, 0.3, 0.55, 0.8],
                     "det_cam": "450,-180,150", "det_lookat": "0,-180,70"},
        "cursor": (15, -600),
        # mesh_rel_z_in: visual-mesh height stays near its -90 base during the roll (small lift
        # allowed). Catches mesh FLOAT/sink geometrically — the failure a 30px player hides from
        # the pixel diff (a 2m float measured only ~2% changed pixels).
        "checks": [{"kind": "roll_capsule_gt", "u": 100},
                   {"kind": "mesh_rel_z_in", "lo": -100, "hi": -40}],
        # Mid-roll deterministic frame: now stable enough to golden-diff the DYNAMIC pose itself.
        # Full-frame ROI: the det capture's residual noise is the PLAYER (tick quantization), so
        # the center crop would concentrate it; the stable fixed-camera background dilutes it.
        "key_frame": "frame_01_det.png",
        "judge_frame": "frame_01_det.png",
        "tol": {"roi": 1.0},
    },
}


def vision_judge(intent, frame_path):
    """Bias-independent semantic check: a SEPARATE headless `claude -p` process that knows only
    the intent + the frame. Returns its VERDICT. Independent of the acting agent by construction."""
    prompt = (
        "You are an independent visual QA judge for a game animation. Judge ONLY from the image.\n"
        f"INTENT: {intent}\n"
        f"Use the Read tool to view this side-view frame: {frame_path}\n"
        "The small silver/metallic figure is the player being judged; a large RED figure if present "
        "is an unrelated enemy — ignore it.\n"
        "Reply with EXACTLY two lines:\n"
        "VERDICT: <MATCHES_INTENT|PARTIAL|DOES_NOT_MATCH>\n"
        "WHY: <one blunt sentence on the player's pose vs the intent>"
    )
    try:
        r = subprocess.run([CLAUDE, "-p", prompt, "--allowedTools", "Read"],
                           capture_output=True, text=True, timeout=300,
                           stdin=subprocess.DEVNULL)  # closed stdin -> no 3s wait; real .exe -> no shell mangling
        out = (r.stdout or "") + (r.stderr or "")
        verdict, why = "UNKNOWN", ""
        for line in out.splitlines():
            if "VERDICT:" in line.upper():
                verdict = line.split(":", 1)[1].strip().split()[0].upper().strip(".")
            elif line.upper().startswith("WHY:"):
                why = line.split(":", 1)[1].strip()
        return {"verdict": verdict, "why": why}
    except Exception as e:
        return {"verdict": "ERROR", "why": str(e)}


def _check(c, m):
    from run_contracts import check
    return check(c, m)


def main():
    if len(sys.argv) < 2 or sys.argv[1] not in GATES:
        print(f"usage: python verify_gate.py <{'|'.join(GATES)}> [--map M] [--promote]")
        return 2
    name = sys.argv[1]
    g = GATES[name]
    promote = "--promote" in sys.argv
    map_path = sys.argv[sys.argv.index("--map") + 1] if "--map" in sys.argv else g["map"]
    os.makedirs(GOLD_DIR, exist_ok=True)
    baseline = json.load(open(GATE_BASELINE)) if os.path.exists(GATE_BASELINE) else {}

    print(f"GATE '{name}' on {map_path}\n  intent: {g['intent']}\n")
    trace, obs, frames = run_scenario("gate_" + name, g["scenario"], map_path)
    m = metrics(trace, obs, g.get("cursor"))

    # 1. NUMERIC
    cks = [_check(c, m) for c in g["checks"]]
    num_pass = all(ok for ok, _ in cks)
    was = baseline.get(name, {}).get("numeric_pass")
    num_status = "REGRESSED" if (was and not num_pass) else ("PASS" if num_pass else "FAIL")
    print(f"  [numeric] {num_status}")
    for ok, desc in cks:
        print(f"     {'+' if ok else '-'} {desc}")

    # 2. VISUAL (golden-image diff of the key frame)
    key_w = os.path.join(OBS_W, "c_gate_" + name, g["key_frame"])
    gold = os.path.join(GOLD_DIR, f"{name}__{g['key_frame']}")
    visual = {"status": "n/a"}
    if not os.path.exists(key_w):
        visual = {"status": "no-key-frame", "expected": key_w}
        print(f"  [visual]  NO KEY FRAME ({g['key_frame']}) — did capture run?")
    elif not os.path.exists(gold):
        visual = {"status": "established", "golden": gold}
        print(f"  [visual]  NO GOLDEN yet — will establish on --promote")
    else:
        d = image_diff.compare(gold, key_w, g.get("tol"))
        visual = {"status": "REGRESSED" if d["regressed"] else "PASS", **d}
        print(f"  [visual]  {visual['status']}  (changed {d['changed_frac']*100:.1f}%/"
              f"{d['changed_frac_budget']*100:.0f}%, mean {d['global_mean']:.3f}/{d['global_mean_budget']})  diff={d['diff_path']}")

    # 3. SEMANTIC (separate vision-judge, auto-invoked)
    judge = {"verdict": "skipped"}
    if "--no-judge" not in sys.argv:
        jf = os.path.join(OBS_W, "c_gate_" + name, g.get("judge_frame", g["key_frame"]))
        if os.path.exists(jf):
            print("  [judge]   invoking independent vision-judge (claude -p)...")
            judge = vision_judge(g["intent"], jf)
            print(f"  [judge]   {judge['verdict']}  {judge.get('why','')}")
        else:
            judge = {"verdict": "no-frame"}
            print(f"  [judge]   NO JUDGE FRAME ({g.get('judge_frame')})")

    # AGGREGATE verdict — all three axes
    reject = (num_status in ("FAIL", "REGRESSED")) or (visual["status"] == "REGRESSED") \
        or (judge["verdict"] == "DOES_NOT_MATCH")
    verdict = {
        "gate": name, "map": map_path, "intent": g["intent"],
        "numeric": {"status": num_status, "checks": [d for _, d in cks], "metrics": m},
        "visual": visual,
        "judge": judge,
        "key_frame": key_w if os.path.exists(key_w) else None,
        "reject": reject,
        "all_frames": frames,
    }
    json.dump(verdict, open(VERDICT, "w"), indent=1)

    # Promote the visual/numeric last-known-good (for regression tracking) when those are clean —
    # independent of the semantic verdict, which reports whether it MATCHES INTENT (may be a known gap).
    if promote and num_status not in ("FAIL", "REGRESSED") and visual["status"] not in ("REGRESSED", "no-key-frame"):
        baseline[name] = {"numeric_pass": num_pass}
        json.dump(baseline, open(GATE_BASELINE, "w"), indent=1)
        if os.path.exists(key_w):
            shutil.copyfile(key_w, gold)
        print(f"\n  PROMOTED -> new last-known-good (baseline + golden {os.path.basename(gold)})")
    print(f"\nVERDICT: {'REJECT' if reject else 'ACCEPT'}  "
          f"[numeric={num_status} | visual={visual['status']} | judge={judge['verdict']}]  Wrote {VERDICT}")
    return 1 if reject else 0


if __name__ == "__main__":
    sys.exit(main())
