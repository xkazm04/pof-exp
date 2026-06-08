"""Toggle bEnableRootMotion on an AnimSequence (diagnostic/fix for root-motion-driven
movement). Call via /pof/python/run {module:"observation.set_root_motion",
function:"run", args:{asset:"/Game/.../Forward_Roll_RT", enable:true}}.

Reports the prior value + frame count so we can tell 'was disabled' from 'is in-place'.
"""
import unreal


def run(args):
    path = args["asset"]
    enable = bool(args.get("enable", True))
    a = unreal.load_asset(path)
    if not a:
        return {"error": "not found", "asset": path}
    info = {"asset": path, "class": a.get_class().get_name()}
    try:
        info["prev_enable_root_motion"] = a.get_editor_property("enable_root_motion")
    except Exception as e:
        info["prev_enable_root_motion"] = f"(err: {e})"
    try:
        a.set_editor_property("enable_root_motion", enable)
        info["set_enable_root_motion"] = enable
    except Exception as e:
        info["set_error"] = str(e)
    try:
        info["force_root_lock"] = a.get_editor_property("root_motion_root_lock")
        info["num_frames"] = a.get_number_of_sampled_keys() if hasattr(a, "get_number_of_sampled_keys") else None
    except Exception:
        pass
    unreal.EditorAssetLibrary.save_loaded_asset(a)
    return info
