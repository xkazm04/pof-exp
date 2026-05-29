"""RunScenario (SP1) — thin Python wrapper over the C++ PoFScenarioRunner harness.

Builds FPoFTimedInput structs from the input specs and starts a PIE session that
injects them over a timeline, leaving PIE live for pose/frame observers.
"""
import unreal

from observation import make_observation


def run(args):
    timed = []
    for spec in args.get("inputs", []):
        ti = unreal.PoFTimedInput()
        ti.set_editor_property("action_path", spec["action"])
        v = spec.get("value", [0, 0])
        ti.set_editor_property("value", unreal.Vector2D(float(v[0]), float(v[1])))
        ti.set_editor_property("start_seconds", float(spec.get("start", 0.0)))
        ti.set_editor_property("duration_seconds", float(spec.get("duration", 1.0)))
        timed.append(ti)
    started = unreal.PoFScenarioRunner.run_scenario(
        args["map"], timed, float(args.get("total_seconds", 1.5)))
    return make_observation("metric", {"started": bool(started)}, scenario_id=args.get("scenario_id"))
