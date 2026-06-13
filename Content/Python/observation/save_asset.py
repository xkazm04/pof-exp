"""Force-persist a loaded asset to disk (the bake's in-memory root-motion edit reported
saved:false — PIE used it, but it must hit disk to survive an editor restart).

Call: /pof/python/run {module:"observation.save_asset", function:"run",
args:{asset:"/Game/Mixamo/Retargeted/SKM_Manny/Forward_Roll_RT"}}.
"""
import unreal


def run(args):
    path = args.get("asset", "/Game/Mixamo/Retargeted/SKM_Manny/Forward_Roll_RT")
    anim = unreal.load_asset(path)
    out = {"path": path, "loaded": bool(anim)}
    if not anim:
        return out
    # Mark dirty so the save isn't skipped as a no-op, then try both save paths.
    try:
        pkg = anim.get_outermost()
        pkg.set_dirty_flag(True)
        out["marked_dirty"] = True
    except Exception as e:
        out["dirty_err"] = str(e)
    try:
        out["saved_loaded"] = unreal.EditorAssetLibrary.save_loaded_asset(anim)
    except Exception as e:
        out["saved_loaded_err"] = str(e)
    try:
        out["saved_path"] = unreal.EditorAssetLibrary.save_asset(path, False)
    except Exception as e:
        out["saved_path_err"] = str(e)
    return out
