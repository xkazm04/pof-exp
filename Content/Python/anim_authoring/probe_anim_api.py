"""Discovery probe for programmatic animation authoring.

Goal: learn the exact UE5.8 Python API for reading/writing AnimSequence bone keyframes, and
dump the mannequin skeleton + idle pose transforms — the foundation for authoring a custom
combat swing entirely in code (no Mixamo clip). Read-only except a tiny validation asset.
"""
import unreal

TAG = "ANIMPROBE"
def log(m):
    unreal.log("%s: %s" % (TAG, m))
def err(m):
    unreal.log_error("%s: %s" % (TAG, m))

IDLE = "/Game/Mixamo/Retargeted/SKM_Manny/Standard_Idle_RT"


def dump_api(obj, label):
    if obj is None:
        log("API[%s] = None" % label); return
    ms = [m for m in dir(obj) if any(k in m.lower() for k in
          ['bone', 'track', 'key', 'frame', 'length', 'rate', 'pose', 'controller', 'model'])]
    log("API[%s/%s]: %s" % (label, type(obj).__name__, ", ".join(ms)))


def main():
    log("BEGIN")

    idle = unreal.load_asset(IDLE)
    if not idle:
        err("idle MISSING at %s — listing retargeted folder" % IDLE)
        ar = unreal.AssetRegistryHelpers.get_asset_registry()
        for a in ar.get_assets_by_path("/Game/Mixamo/Retargeted/SKM_Manny", recursive=True):
            log("  found: %s" % a.get_full_name())
        # also try a plain SK_Mannequin
        for p in ["/Game/Characters/Mannequins/Meshes/SK_Mannequin",
                  "/Game/Characters/Mannequins/Meshes/SKM_Manny"]:
            if unreal.EditorAssetLibrary.does_asset_exist(p):
                log("  mesh/skel exists: %s" % p)
        return

    log("idle class = %s" % type(idle).__name__)
    skel = idle.get_skeleton()
    log("skeleton = %s" % (skel.get_path_name() if skel else "None"))
    dump_api(idle, "AnimSequence")

    # ---- data model (read) ----
    model = None
    for getter in ["get_data_model", "get_data_model_interface"]:
        if hasattr(idle, getter):
            try:
                model = getattr(idle, getter)()
                log("model via %s() -> %s" % (getter, type(model).__name__))
                break
            except Exception as e:
                log("%s() failed: %s" % (getter, e))
    dump_api(model, "DataModel")

    # ---- controller (write) ----
    ctrl = None
    if hasattr(idle, "get_controller"):
        try:
            ctrl = idle.get_controller()
            dump_api(ctrl, "Controller")
        except Exception as e:
            log("get_controller() failed: %s" % e)

    # ---- bone names ----
    names = []
    if model:
        for cand in ["get_bone_track_names", "get_bone_animation_track_names"]:
            if hasattr(model, cand):
                try:
                    names = list(getattr(model, cand)())
                    log("bones via %s: count=%d" % (cand, len(names)))
                    break
                except Exception as e:
                    log("%s failed: %s" % (cand, e))
    if names:
        log("BONES = %s" % ", ".join(str(n) for n in names))

    # ---- read upperarm_r idle keys to learn the key shape ----
    target = None
    for n in names:
        if str(n) == "upperarm_r":
            target = n; break
    if target is not None and model:
        for cand in ["get_bone_track_keys", "get_bone_pose_for_frame",
                     "get_bone_track_transforms", "evaluate_bone_track_transform"]:
            if hasattr(model, cand):
                log("model has reader: %s" % cand)
        try:
            keys = model.get_bone_track_keys(target)
            log("upperarm_r get_bone_track_keys -> type=%s" % type(keys).__name__)
            # try to introspect the returned structure
            for attr in ["positional_keys", "rotational_keys", "scaling_keys"]:
                if hasattr(keys, attr):
                    arr = getattr(keys, attr)
                    log("  .%s len=%s first=%s" % (attr, len(arr), arr[0] if len(arr) else "-"))
        except Exception as e:
            log("get_bone_track_keys(upperarm_r) failed: %s" % e)

    # ---- number of frames / play length of idle ----
    if model:
        for cand in ["get_number_of_frames", "get_number_of_keys", "get_play_length", "get_frame_rate"]:
            if hasattr(model, cand):
                try:
                    log("idle.%s = %s" % (cand, getattr(model, cand)()))
                except Exception as e:
                    log("%s failed: %s" % (cand, e))

    log("DONE")

main()
