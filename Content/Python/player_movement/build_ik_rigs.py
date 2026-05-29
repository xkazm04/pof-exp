"""Step 04 — Build IK_Mixamo + IK_Manny + RTG_MixamoToManny (UE 5.7 IK API).

Uses the engine's auto-template detection: both the Mixamo (X Bot) and UE5 Manny
skeletons match known templates, so `apply_auto_generated_retarget_definition()`
generates the retarget chains + pelvis automatically — far more robust than
hand-coded bone-name chains. Idempotent: skips assets that already exist.
"""

import unreal


DEST = "/Game/Characters/Player/IK"

# Source skeletal MESHES (the 5.7 IK rig binds to a mesh, not a skeleton).
XBOT_MESH = "/Game/Mixamo/Raw/Standard_Idle"
MANNY_MESH_CANDIDATES = [
    "/MoverTests/Characters/Mannequins/Meshes/SKM_Manny",
    "/MoverExamples/Characters/Mannequins/Meshes/SKM_Manny_Simple",
]


def _find_manny_mesh():
    for p in MANNY_MESH_CANDIDATES:
        if unreal.EditorAssetLibrary.does_asset_exist(p):
            return p
    return None


def _build_ik_rig(name, mesh_path):
    """Create an IKRigDefinition for a skeletal mesh + auto-generate chains.
    Returns (was_created, asset_or_none)."""
    asset_path = f"{DEST}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        return False, unreal.EditorAssetLibrary.load_asset(asset_path)

    mesh = unreal.EditorAssetLibrary.load_asset(mesh_path)
    if not mesh:
        raise RuntimeError(f"skeletal mesh not found: {mesh_path}")

    tools = unreal.AssetToolsHelpers.get_asset_tools()
    factory = unreal.IKRigDefinitionFactory()
    rig = tools.create_asset(name, DEST, unreal.IKRigDefinition, factory)
    if not rig:
        raise RuntimeError(f"create_asset failed for IK rig {name}")

    ctrl = unreal.IKRigController.get_controller(rig)
    ctrl.set_skeletal_mesh(mesh)
    matched = ctrl.apply_auto_generated_retarget_definition()
    if not matched:
        unreal.log_warning(
            f"[build_ik_rigs] {name}: no known skeletal template matched — "
            f"retarget chains may be incomplete"
        )
    unreal.EditorAssetLibrary.save_asset(asset_path)
    return True, rig


def _build_retargeter(name, src_rig, tgt_rig):
    asset_path = f"{DEST}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        return False, unreal.EditorAssetLibrary.load_asset(asset_path)

    tools = unreal.AssetToolsHelpers.get_asset_tools()
    rtg = tools.create_asset(name, DEST, unreal.IKRetargeter, None)
    if not rtg:
        raise RuntimeError(f"create_asset failed for retargeter {name}")

    ctrl = unreal.IKRetargeterController.get_controller(rtg)
    ctrl.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE, src_rig)
    ctrl.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET, tgt_rig)
    ctrl.add_default_ops()  # Pelvis Motion, FK Chains, IK Chains, IK Solve, Root Motion
    ctrl.auto_map_chains(unreal.AutoMapChainType.FUZZY, True)
    unreal.EditorAssetLibrary.save_asset(asset_path)
    return True, rtg


def run(args):
    result = {"created": [], "skipped": [], "failed": []}
    try:
        if not unreal.EditorAssetLibrary.does_asset_exist(XBOT_MESH):
            result["failed"].append(f"Mixamo X Bot mesh not imported at {XBOT_MESH}; run step 03 first")
            return result
        manny_mesh = _find_manny_mesh()
        if not manny_mesh:
            result["failed"].append("UE5 Manny skeletal mesh not found (MoverTests/MoverExamples)")
            return result

        created, ik_mixamo = _build_ik_rig("IK_Mixamo", XBOT_MESH)
        (result["created"] if created else result["skipped"]).append("IK_Mixamo")
        created, ik_manny = _build_ik_rig("IK_Manny", manny_mesh)
        (result["created"] if created else result["skipped"]).append("IK_Manny")

        if not ik_mixamo or not ik_manny:
            result["failed"].append("IK rig load failed; cannot build retargeter")
            return result

        created, _ = _build_retargeter("RTG_MixamoToManny", ik_mixamo, ik_manny)
        (result["created"] if created else result["skipped"]).append("RTG_MixamoToManny")
    except Exception as e:  # noqa: BLE001
        result["failed"].append(str(e))
    return result
