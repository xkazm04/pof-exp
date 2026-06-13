"""Replace a roll clip's jerky/rotating root motion with a SMOOTH, STRAIGHT, forward-only
root-motion track (a smootherstep ease over the clip's natural net distance, zero rotation,
zero vertical). The body still visually rolls (limb bones unchanged) but the capsule travels
smoothly in a straight line — fixing the ±jerk + 151deg spin that are baked into the Mixamo clip.

Call: /pof/python/run {module:"observation.bake_clean_roll_root_motion", function:"run",
args:{asset:"/Game/Mixamo/Retargeted/SKM_Manny/Forward_Roll_RT"}}.

Reports each step so a failure pinpoints exactly which anim-data API gives out.
"""
import math

import unreal


def _smootherstep(x):
    x = 0.0 if x < 0 else (1.0 if x > 1 else x)
    return x * x * x * (x * (x * 6 - 15) + 10)


def run(args):
    out = {}
    path = args.get("asset", "/Game/Mixamo/Retargeted/SKM_Manny/Forward_Roll_RT")
    anim = unreal.load_asset(path)
    out["loaded"] = bool(anim) and isinstance(anim, unreal.AnimSequence)
    if not out["loaded"]:
        return {**out, "error": "not an AnimSequence", "path": path}

    length = float(anim.get_play_length())
    out["length"] = length

    # Bake a fixed forward distance along the clip's forward axis (+X local in UE; flip via
    # args.axis if the roll ends up sideways). Matches the measured natural roll travel.
    dist = float(args.get("dist", 480.0))
    axis = args.get("axis", "x")
    fwd = {"x": unreal.Vector(1, 0, 0), "-x": unreal.Vector(-1, 0, 0),
           "y": unreal.Vector(0, 1, 0), "-y": unreal.Vector(0, -1, 0)}.get(axis, unreal.Vector(1, 0, 0))
    out["dist"] = dist
    out["axis"] = axis

    # --- frame count via AnimationLibrary (get_data_model isn't exposed in 5.7) ---
    try:
        n = unreal.AnimationLibrary.get_num_frames(anim)
        out["frames"] = n
    except Exception as e:
        return {**out, "error": f"get_num_frames failed: {e}"}

    # --- build a smooth straight forward curve on the root bone ---
    positions, rotations, scales = [], [], []
    ident = unreal.Quat(0, 0, 0, 1)
    one = unreal.Vector(1, 1, 1)
    # Optional vertical arc: raise the whole roll mid-flight (feet are tucked there) so the
    # skinned mesh clears the floor, settling to 0 at both ends (no start/end float).
    lift = float(args.get("lift", 0.0))
    keys = n + 1  # UE bone tracks are usually frames+1 keys
    out["lift"] = lift
    for i in range(keys):
        p = i / max(1, keys - 1)
        a = _smootherstep(p)
        z = lift * math.sin(math.pi * p)  # 0 -> peak at mid -> 0
        positions.append(unreal.Vector(fwd.x * dist * a, fwd.y * dist * a, z))
        rotations.append(ident)
        scales.append(one)

    # --- acquire the data controller (5.7 exposes it via attribute, not get_controller()) ---
    controller = None
    tried = {}
    for name, getter in [
        ("controller()", lambda: anim.controller()),
        ("data_model_interface()", lambda: anim.data_model_interface()),
        ("get_editor_property(controller)", lambda: anim.get_editor_property("controller")),
        ("get_editor_property(data_model_interface)", lambda: anim.get_editor_property("data_model_interface")),
    ]:
        try:
            c = getter()
            if c:
                controller = c
                out["controller_via"] = name
                break
        except Exception as e:
            tried[name] = str(e)
    if controller is None:
        return {**out, "error": "no controller accessor worked", "tried": tried}

    # --- write the root track ---
    try:
        if hasattr(controller, "open_bracket"):
            controller.open_bracket("Bake clean roll root motion")
        controller.set_bone_track_keys(unreal.Name("root"), positions, rotations, scales)
        if hasattr(controller, "close_bracket"):
            controller.close_bracket()
        out["wrote_track"] = True
    except Exception as e:
        out["set_bone_track_keys_error"] = str(e)
        out["controller_methods"] = sorted(m for m in dir(controller) if "bone" in m.lower() or "track" in m.lower() or "key" in m.lower())
        out["wrote_track"] = False

    # --- enable root motion, no rotation lock that would kill translation ---
    try:
        anim.set_editor_property("enable_root_motion", True)
        anim.set_editor_property("force_root_lock", False)
    except Exception as e:
        out["enable_rm_error"] = str(e)
    out["saved"] = unreal.EditorAssetLibrary.save_loaded_asset(anim)
    return out
