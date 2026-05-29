"""RunScenarioEx (verification harness) — wraps UPoFScenarioRunner.RunScenarioEx.

Injects timed inputs (use {"key":"W"} for REAL key injection through the IMC, or
{"action":"/Game/.../IA_Move","value":[x,y]} for action-level injection) and returns
time-sampled EVALUATED-pose observations (arm droop, location, speed, frame path) read
INSIDE the PIE tick loop — so 'is it walking / T-posing / does WASD move it' is answered
from ground truth, not a proxy. droopL/R ~0deg = arms horizontal (T-POSE); ~50-80 = down.
"""
import unreal

from observation import make_observation


def run(args):
    timed = []
    for spec in args.get("inputs", []):
        ti = unreal.PoFTimedInput()
        if spec.get("key"):
            ti.set_editor_property("key", spec["key"])
        else:
            ti.set_editor_property("action_path", spec.get("action", ""))
            v = spec.get("value", [0, 0])
            ti.set_editor_property("value", unreal.Vector2D(float(v[0]), float(v[1])))
        ti.set_editor_property("start_seconds", float(spec.get("start", 0.0)))
        ti.set_editor_property("duration_seconds", float(spec.get("duration", 1.0)))
        timed.append(ti)

    res = unreal.PoFScenarioRunner.run_scenario_ex(
        args["map"], timed, float(args.get("total_seconds", 2.0)),
        args.get("frame_dir", ""), int(args.get("num_samples", 4)))

    samples = []
    for s in res.get_editor_property("samples"):
        loc = s.get_editor_property("location")
        samples.append({
            "t": round(s.get_editor_property("time"), 2),
            "loc": [round(loc.x, 1), round(loc.y, 1), round(loc.z, 1)],
            "speed": round(s.get_editor_property("speed"), 1),
            "droopL": round(s.get_editor_property("arm_droop_left_deg"), 1),
            "droopR": round(s.get_editor_property("arm_droop_right_deg"), 1),
            "pose_valid": bool(s.get_editor_property("pose_valid")),
            "frame": str(s.get_editor_property("frame")),
        })
    data = {"started": bool(res.get_editor_property("started")),
            "error": str(res.get_editor_property("error")),
            "samples": samples}
    return make_observation("metric", data, scenario_id=args.get("scenario_id"))
