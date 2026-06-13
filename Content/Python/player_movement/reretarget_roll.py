"""Force a CLEAN re-retarget of just Forward_Roll (X Bot -> Manny), overwriting the
hacked Forward_Roll_RT to restore the clip's NATURAL, pose-synced root motion.

The normal batch retarget (retarget.py) skips clips whose _RT already exists; this
deletes the existing RT first so the natural retarget is regenerated. Self-contained
(no cross-module import) so it runs headless via the bridge.

Call: /pof/python/run {module:"player_movement.reretarget_roll", function:"run", args:{}}.
"""
import unreal

RTG = "/Game/Characters/Player/IK/RTG_MixamoToManny"
SRC_MESH = "/Game/Mixamo/Raw/Standard_Idle"
TGT_CANDIDATES = [
    "/MoverTests/Characters/Mannequins/Meshes/SKM_Manny",
    "/MoverExamples/Characters/Mannequins/Meshes/SKM_Manny_Simple",
]
RT = "/Game/Mixamo/Retargeted/SKM_Manny/Forward_Roll_RT"


def run(args):
    out = {}
    rtg = unreal.EditorAssetLibrary.load_asset(RTG)
    src = unreal.EditorAssetLibrary.load_asset(SRC_MESH)
    tgt = None
    for p in TGT_CANDIDATES:
        tgt = unreal.EditorAssetLibrary.load_asset(p)
        if tgt:
            break
    if not (rtg and src and tgt):
        return {"error": "missing rtg/src/tgt", "rtg": bool(rtg), "src": bool(src), "tgt": bool(tgt)}

    ar = unreal.AssetRegistryHelpers.get_asset_registry()
    f = unreal.ARFilter(package_paths=["/Game/Mixamo/Raw"], class_names=["AnimSequence"], recursive_paths=False)
    raw_ad = next((a for a in ar.get_assets(f) if str(a.asset_name) == "Forward_Roll"), None)
    if raw_ad is None:
        return {"error": "Raw/Forward_Roll not found"}

    if unreal.EditorAssetLibrary.does_asset_exist(RT):
        out["deleted_hacked_rt"] = unreal.EditorAssetLibrary.delete_asset(RT)

    unreal.IKRetargetBatchOperation.duplicate_and_retarget(
        [raw_ad], src, tgt, rtg, suffix="_RT",
        include_referenced_assets=False, overwrite_existing_files=True,
    )
    root_path = "/Game/Forward_Roll_RT"  # duplicate_and_retarget always writes to /Game root
    out["root_made"] = unreal.EditorAssetLibrary.does_asset_exist(root_path)
    if out["root_made"]:
        if unreal.EditorAssetLibrary.does_asset_exist(RT):
            unreal.EditorAssetLibrary.delete_asset(RT)
        unreal.EditorAssetLibrary.rename_asset(root_path, RT)
    out["rt_exists"] = unreal.EditorAssetLibrary.does_asset_exist(RT)
    # Confirm it has real root motion (natural retarget should), and report length.
    anim = unreal.EditorAssetLibrary.load_asset(RT)
    if anim:
        try:
            out["enable_root_motion"] = anim.get_editor_property("enable_root_motion")
            out["length"] = float(anim.get_play_length())
        except Exception as e:
            out["probe_err"] = str(e)
    unreal.EditorAssetLibrary.save_directory("/Game/Mixamo/Retargeted/SKM_Manny", only_if_is_dirty=False)
    return out
