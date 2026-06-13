"""Movement contract suite — the closed verification loop.

HOST-side (plain stdlib). Runs a battery of movement CONTRACTS through the live harness
(bridge :30040 + scenario_pie), evaluates each against metric thresholds -> PASS/FAIL,
diffs against a saved baseline -> REGRESSED, and writes a report. This is what catches
regressions (a prior-passing behavior breaking) automatically and makes "functional"
machine-checkable.

Run:  python run_contracts.py [--map /Game/Maps/TestHarness] [--update-baseline]

Each contract = a scenario (cursor + key/event timeline) + checks. Metrics come from the
per-tick trace.json (yaw, position, distance, foot-slide) and the sampled observations.json
(droopL/R, anim_direction). Frames are listed for visual review.
"""
import json
import math
import os
import sys
import time
import urllib.request

BRIDGE = "http://localhost:30040/pof/python/run"
OBS_W = r"C:\Users\kazda\AppData\Local\Temp\pof_obs"
OBS_F = "C:/Users/kazda/AppData/Local/Temp/pof_obs"
INBOX = os.path.join(OBS_W, "inbox.json")
BASELINE = os.path.join(OBS_W, "baseline.json")
REPORT = os.path.join(OBS_W, "report.json")
DEFAULT_MAP = "/Game/Maps/TestHarness"


def post(module, function, args):
    body = json.dumps({"module": module, "function": function, "args": args}).encode()
    req = urllib.request.Request(BRIDGE, data=body, headers={"Content-Type": "application/json"})
    with urllib.request.urlopen(req, timeout=60) as r:
        return json.load(r)


def run_scenario(name, scenario, map_path):
    out_w = os.path.join(OBS_W, "c_" + name)
    out_f = OBS_F + "/c_" + name
    if os.path.isdir(out_w):
        for f in os.listdir(out_w):
            try:
                os.remove(os.path.join(out_w, f))
            except OSError:
                pass
    else:
        os.makedirs(out_w, exist_ok=True)
    inbox = dict(scenario)
    inbox["out_dir"] = out_f
    with open(INBOX, "w") as f:
        json.dump(inbox, f)
    post("observation.scenario_pie", "start", {"map": map_path})
    done = os.path.join(out_w, "DONE")
    for _ in range(40):
        if os.path.exists(done):
            break
        time.sleep(0.5)
    post("observation.scenario_pie", "stop", {})
    trace = obs = None
    tp = os.path.join(out_w, "trace.json")
    op = os.path.join(out_w, "observations.json")
    if os.path.exists(tp):
        trace = json.load(open(tp)).get("trace", [])
    if os.path.exists(op):
        obs = json.load(open(op)).get("samples", [])
    frames = [f for f in os.listdir(out_w) if f.endswith(".png")] if os.path.isdir(out_w) else []
    return trace, obs, [out_f + "/" + f for f in sorted(frames)]


def metrics(trace, obs, cursor):
    m = {}
    if trace:
        last = trace[-1]
        m["final_x"], m["final_y"], m["final_yaw"] = last.get("x", 0), last.get("y", 0), last.get("yaw", 0)
        m["peak_speed"] = max(r.get("speed", 0) for r in trace)
        m["total_capsule"] = sum(math.hypot(r.get("dCapX", 0), r.get("dCapY", 0)) for r in trace)
        roll = [r for r in trace if r.get("montage")]
        m["roll_capsule"] = sum(math.hypot(r.get("dCapX", 0), r.get("dCapY", 0)) for r in roll)
        # net displacement direction (heading) over the whole run
        dx = m["final_x"] - trace[0].get("x", 0)
        dy = m["final_y"] - trace[0].get("y", 0)
        m["heading_deg"] = math.degrees(math.atan2(dy, dx))
        m["net_disp"] = math.hypot(dx, dy)
        # How far the BODY swept its facing over the run (real-path cursor responsiveness).
        yaws = [r.get("yaw", 0) for r in trace]
        f0 = (math.cos(math.radians(yaws[0])), math.sin(math.radians(yaws[0])))
        sweep = 0.0
        for yw in yaws:
            fi = (math.cos(math.radians(yw)), math.sin(math.radians(yw)))
            sweep = max(sweep, math.degrees(math.acos(max(-1, min(1, f0[0] * fi[0] + f0[1] * fi[1])))))
        m["yaw_sweep"] = sweep
        mz = [r.get("meshMinZ") for r in trace if r.get("meshMinZ") is not None]
        if mz:
            m["mesh_min_z"] = min(mz)
        # Mesh relative-Z during the dodge montage: catches the visual mesh FLOATING (lift bug —
        # a 2m float read as only ~2% pixel change, under the visual budget) or sinking. Geometric
        # failure -> numeric axis, where a 30px-in-frame player can't hide.
        drz = [r.get("meshRelZ") for r in trace if r.get("montage") and r.get("meshRelZ") is not None]
        if drz:
            m["dodge_mesh_rel_z_max"] = max(drz)
            m["dodge_mesh_rel_z_min"] = min(drz)
        if cursor is not None:
            cx, cy = cursor
            tox, toy = cx - m["final_x"], cy - m["final_y"]
            fwd = (math.cos(math.radians(m["final_yaw"])), math.sin(math.radians(m["final_yaw"])))
            tol = math.hypot(tox, toy)
            if tol > 1:
                dot = (fwd[0] * tox + fwd[1] * toy) / tol
                m["facing_error_deg"] = math.degrees(math.acos(max(-1, min(1, dot))))
    if obs:
        droops = [s.get("droopL", 0) for s in obs] + [s.get("droopR", 0) for s in obs]
        m["droop_min"], m["droop_max"] = min(droops), max(droops)
        m["droop_var"] = m["droop_max"] - m["droop_min"]
        m["final_droop"] = (obs[-1].get("droopL", 0) + obs[-1].get("droopR", 0)) / 2
        moving = [s for s in obs if s.get("speed", 0) > 50]
        if moving:
            m["anim_dir_move"] = max(abs(s.get("anim_direction", 0)) for s in moving)
        # post-roll: samples after the montage stops, while still moving
        post_roll = [s for s in obs if not s.get("montage_playing") and s.get("speed", 0) > 50]
        if post_roll:
            m["post_roll_anim_dir"] = max(abs(s.get("anim_direction", 0)) for s in post_roll)
    return m


def check(c, m):
    k = c["kind"]
    if k == "facing_error_lt":
        v = m.get("facing_error_deg")
        return (v is not None and v < c["deg"], f"facing_error={v:.0f}deg (<{c['deg']})" if v is not None else "no facing data")
    if k == "droop_in":
        v = m.get("final_droop")
        return (v is not None and c["lo"] <= v <= c["hi"], f"droop={v:.0f} (in {c['lo']}-{c['hi']})" if v is not None else "no droop")
    if k == "droop_var_gt":
        v = m.get("droop_var")
        return (v is not None and v > c["v"], f"droop_var={v:.0f} (>{c['v']})" if v is not None else "no droop")
    if k == "capsule_dist_gt":
        v = m.get("total_capsule", 0)
        return (v > c["u"], f"capsule={v:.0f}u (>{c['u']})")
    if k == "roll_capsule_gt":
        v = m.get("roll_capsule", 0)
        return (v > c["u"], f"roll_capsule={v:.0f}u (>{c['u']})")
    if k == "anim_dir_abs_in":
        v = m.get("anim_dir_move")
        return (v is not None and c["lo"] <= v <= c["hi"], f"|anim_dir|={v:.0f} (in {c['lo']}-{c['hi']})" if v is not None else "no anim_dir")
    if k == "post_roll_anim_dir_gt":
        v = m.get("post_roll_anim_dir")
        return (v is not None and v > c["deg"], f"post_roll |anim_dir|={v:.0f} (>{c['deg']})" if v is not None else "no post-roll move")
    if k == "yaw_sweep_gt":
        v = m.get("yaw_sweep")
        return (v is not None and v > c["deg"], f"yaw_sweep={v:.0f}deg (>{c['deg']})" if v is not None else "no yaw")
    if k == "mesh_min_z_gt":
        v = m.get("mesh_min_z")
        return (v is not None and v > c["z"], f"mesh_min_z={v:.0f} (>{c['z']})" if v is not None else "no meshZ")
    if k == "mesh_rel_z_in":
        lo, hi = m.get("dodge_mesh_rel_z_min"), m.get("dodge_mesh_rel_z_max")
        if lo is None:
            return (False, "no dodge meshRelZ")
        ok = c["lo"] <= lo and hi <= c["hi"]
        return (ok, f"dodge meshRelZ {lo:.0f}..{hi:.0f} (in {c['lo']}..{c['hi']})")
    return (False, f"unknown check {k}")


C = "0,2000,90"
CONTRACTS = [
    {"name": "faces-cursor", "cursor": (1000, 0), "total": 1.6, "samples": 4,
     "inputs": [{"event": "set_cursor", "event_arg": "1000,0,90", "start": 0.0}],
     "checks": [{"kind": "facing_error_lt", "deg": 25}]},
    {"name": "cursor-responds-to-mouse", "cursor": None, "total": 1.9, "samples": 4,
     "inputs": [{"event": "set_mouse", "event_arg": "1400,500", "start": 0.0}, {"event": "set_mouse", "event_arg": "300,500", "start": 0.95}],
     "checks": [{"kind": "yaw_sweep_gt", "deg": 30}]},
    {"name": "idle-not-tpose", "cursor": None, "total": 1.6, "samples": 4,
     "inputs": [], "checks": [{"kind": "droop_in", "lo": 45, "hi": 90}]},
    {"name": "wasd-W-moves", "cursor": None, "total": 1.8, "samples": 4,
     "inputs": [{"key": "W", "start": 0.3, "duration": 1.2}],
     "checks": [{"kind": "capsule_dist_gt", "u": 300}]},
    {"name": "strafe-animates", "cursor": (0, 2000), "total": 2.2, "samples": 6,
     "inputs": [{"event": "set_cursor", "event_arg": C, "start": 0.0}, {"key": "W", "start": 0.5, "duration": 1.5}],
     "checks": [{"kind": "anim_dir_abs_in", "lo": 45, "hi": 140}, {"kind": "droop_var_gt", "v": 6}]},
    # NOTE: no mesh_min_z assertion here. The mesh BOUNDS box balloons during the horizontal
    # roll, so it reports ~-58 even when the body is visually at floor level — a conservative-
    # bounds artifact, not a real clip (confirmed via side-view). A trustworthy floor-clip
    # metric needs the lowest relevant BONE (foot/hand/head), calibrated idle-vs-roll — TODO.
    {"name": "roll-travels", "cursor": (0, 2000), "total": 2.5, "samples": 6,
     "inputs": [{"event": "set_cursor", "event_arg": C, "start": 0.0}, {"key": "SpaceBar", "start": 0.8, "duration": 0.1}],
     "checks": [{"kind": "roll_capsule_gt", "u": 100}]},
    {"name": "roll-recovers", "cursor": (0, 2000), "total": 2.8, "samples": 8,
     "inputs": [{"event": "set_cursor", "event_arg": C, "start": 0.0}, {"key": "SpaceBar", "start": 0.8, "duration": 0.1}],
     "checks": [{"kind": "droop_in", "lo": 45, "hi": 90}]},
    {"name": "mouse-aim-survives-roll", "cursor": (0, 2000), "total": 3.0, "samples": 10,
     "inputs": [{"event": "set_cursor", "event_arg": C, "start": 0.0}, {"key": "W", "start": 0.5, "duration": 2.2}, {"key": "SpaceBar", "start": 0.8, "duration": 0.1}],
     "checks": [{"kind": "post_roll_anim_dir_gt", "deg": 25}]},
]


def main():
    map_path = DEFAULT_MAP
    update_baseline = "--update-baseline" in sys.argv
    if "--map" in sys.argv:
        map_path = sys.argv[sys.argv.index("--map") + 1]
    baseline = json.load(open(BASELINE)) if os.path.exists(BASELINE) else {}
    results = {}
    print(f"Running {len(CONTRACTS)} contracts on {map_path}\n")
    for con in CONTRACTS:
        scn = {"inputs": con["inputs"], "total_seconds": con["total"], "num_samples": con["samples"], "settle": 0.5}
        trace, obs, frames = run_scenario(con["name"], scn, map_path)
        m = metrics(trace, obs, con.get("cursor"))
        cks = [(check(c, m)) for c in con["checks"]]
        passed = all(ok for ok, _ in cks)
        was = baseline.get(con["name"], {}).get("passed")
        status = "PASS" if passed else "FAIL"
        if was is True and not passed:
            status = "REGRESSED"
        results[con["name"]] = {"passed": passed, "checks": [d for _, d in cks], "metrics": m, "frames": frames}
        mark = {"PASS": "[ok]", "FAIL": "[XX]", "REGRESSED": "[!!]"}[status]
        print(f"{mark} {con['name']:<26} {status}")
        for ok, desc in cks:
            print(f"       {'+' if ok else '-'} {desc}")
    json.dump({k: {"passed": v["passed"]} for k, v in results.items()}, open(REPORT + ".tmp", "w"))
    json.dump(results, open(REPORT, "w"), indent=1)
    if update_baseline:
        json.dump({k: {"passed": v["passed"]} for k, v in results.items()}, open(BASELINE, "w"), indent=1)
        print("\n(baseline updated)")
    n_pass = sum(1 for v in results.values() if v["passed"])
    print(f"\n{n_pass}/{len(CONTRACTS)} passed. Report: {REPORT}")


if __name__ == "__main__":
    main()
