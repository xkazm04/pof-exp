"""Step 05 — Batch retarget Mixamo clips onto the Manny skeleton (UE 5.7).

Uses unreal.IKRetargetBatchOperation.duplicate_and_retarget: duplicates each
AnimSequence in /Game/Mixamo/Raw/ and retargets X Bot → Manny via
RTG_MixamoToManny. The search/replace + suffix relocate the output to
/Game/Mixamo/Retargeted/SKM_Manny/<Name>_RT. Idempotent.
"""

import unreal


SRC_DIR = "/Game/Mixamo/Raw"
DST_DIR = "/Game/Mixamo/Retargeted/SKM_Manny"
RETARGETER_PATH = "/Game/Characters/Player/IK/RTG_MixamoToManny"
SRC_MESH = "/Game/Mixamo/Raw/Standard_Idle"
TGT_MESH_CANDIDATES = [
    "/MoverTests/Characters/Mannequins/Meshes/SKM_Manny",
    "/MoverExamples/Characters/Mannequins/Meshes/SKM_Manny_Simple",
]


def _anim_assetdatas():
    """Return AssetData for every AnimSequence under SRC_DIR that isn't yet retargeted."""
    ar = unreal.AssetRegistryHelpers.get_asset_registry()
    f = unreal.ARFilter(package_paths=[SRC_DIR], class_names=["AnimSequence"], recursive_paths=False)
    pending = []
    for a in ar.get_assets(f):
        name = str(a.asset_name)
        if unreal.EditorAssetLibrary.does_asset_exist(f"{DST_DIR}/{name}_RT"):
            continue
        pending.append(a)
    return pending


def run(args):
    result = {"created": [], "skipped": [], "failed": []}
    try:
        rtg = unreal.EditorAssetLibrary.load_asset(RETARGETER_PATH)
        if not rtg:
            result["failed"].append(f"retargeter missing: {RETARGETER_PATH}; run step 04 first")
            return result
        src_mesh = unreal.EditorAssetLibrary.load_asset(SRC_MESH)
        tgt_mesh = None
        for p in TGT_MESH_CANDIDATES:
            tgt_mesh = unreal.EditorAssetLibrary.load_asset(p)
            if tgt_mesh:
                break
        if not src_mesh or not tgt_mesh:
            result["failed"].append("source (X Bot) or target (Manny) skeletal mesh not found")
            return result

        # Mark already-retargeted clips as skipped.
        ar = unreal.AssetRegistryHelpers.get_asset_registry()
        f = unreal.ARFilter(package_paths=[SRC_DIR], class_names=["AnimSequence"], recursive_paths=False)
        all_anims = list(ar.get_assets(f))
        pending = []
        for a in all_anims:
            name = str(a.asset_name)
            if unreal.EditorAssetLibrary.does_asset_exist(f"{DST_DIR}/{name}_RT"):
                result["skipped"].append(name)
            else:
                pending.append(a)

        if pending:
            # duplicate_and_retarget always outputs to /Game/<Name><suffix> (root) —
            # it doesn't honour a target folder. So create there, then relocate each
            # into DST_DIR, normalising the rig-clip's "_Anim" out of the name so the
            # blend-space name-matching (Standard_Idle_RT) lines up.
            created_assets = unreal.IKRetargetBatchOperation.duplicate_and_retarget(
                pending, src_mesh, tgt_mesh, rtg, suffix="_RT",
                include_referenced_assets=False, overwrite_existing_files=True,
            )
            for a in pending:
                src_name = str(a.asset_name)              # e.g. "Walking" or "Standard_Idle_Anim"
                root_path = f"/Game/{src_name}_RT"          # where duplicate_and_retarget put it
                clean = src_name[:-5] if src_name.endswith("_Anim") else src_name
                dst_path = f"{DST_DIR}/{clean}_RT"
                if not unreal.EditorAssetLibrary.does_asset_exist(root_path):
                    result["failed"].append(f"retarget produced no asset for {src_name}")
                    continue
                if unreal.EditorAssetLibrary.does_asset_exist(dst_path):
                    unreal.EditorAssetLibrary.delete_asset(dst_path)
                unreal.EditorAssetLibrary.rename_asset(root_path, dst_path)
                result["created"].append(f"{clean}_RT")
            unreal.EditorAssetLibrary.save_directory(DST_DIR, only_if_is_dirty=False)
    except Exception as e:  # noqa: BLE001
        result["failed"].append(str(e))
    return result
