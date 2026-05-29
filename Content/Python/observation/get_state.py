"""GetState (SP1) — semantic asset introspection (T3, non-PIE).

Reports MEANING not just "property set": anim length/keyframes, blend-space
sample count + skeleton, anim-blueprint target skeleton.
"""
import unreal

from observation import make_observation


def run(args):
    path = args["asset_path"]
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not asset:
        return make_observation("state", {"path": path, "error": "asset not found"})
    cls = asset.get_class().get_name()
    data = {"path": path, "class": cls}

    if cls in ("AnimSequence", "AnimSequenceBase", "AnimMontage"):
        data["length"] = asset.get_play_length()
        al = getattr(unreal, "AnimationLibrary", None)
        if al and hasattr(al, "get_num_keys"):
            try:
                data["num_keys"] = al.get_num_keys(asset)
            except Exception:
                pass
        if al and hasattr(al, "get_num_frames"):
            try:
                data["num_frames"] = al.get_num_frames(asset)
            except Exception:
                pass
    elif cls == "BlendSpace":
        try:
            data["sample_count"] = len(asset.get_editor_property("sample_data"))
        except Exception:
            data["sample_count"] = None
        skel = asset.get_skeleton()
        data["skeleton"] = str(skel.get_path_name()) if skel else None
    elif cls == "AnimBlueprint":
        try:
            ts = asset.get_editor_property("target_skeleton")
            data["target_skeleton"] = str(ts.get_path_name()) if ts else None
        except Exception:
            data["target_skeleton"] = None

    return make_observation("state", data)
