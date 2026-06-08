"""Set a montage's intrinsic RateScale (asset-level playback speed — applied at evaluation,
unlike the PlayMontage Rate param which wasn't propagating from the BP CDO to PIE spawns).
Call: /pof/python/run {module:"observation.set_montage_rate", function:"run",
args:{asset:"/Game/.../AM_Roll", rate:1.5}}.
"""
import unreal


def run(args):
    a = unreal.load_asset(args["asset"])
    if not a:
        return {"error": "not found", "asset": args["asset"]}
    prev = None
    try:
        prev = a.get_editor_property("rate_scale")
    except Exception as e:
        prev = f"(err {e})"
    a.set_editor_property("rate_scale", float(args["rate"]))
    saved = unreal.EditorAssetLibrary.save_loaded_asset(a)
    return {"asset": args["asset"], "prev_rate_scale": prev, "set_rate_scale": float(args["rate"]), "saved": saved}
