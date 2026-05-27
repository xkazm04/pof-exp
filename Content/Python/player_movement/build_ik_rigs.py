"""Step 04 — Build IK_Mixamo + IK_Manny + RTG_MixamoToManny.

Idempotent: skips assets that already exist. The IK rigs use the deterministic
Mixamo X-Bot bone names and the standard UE5 Manny bone names. Retarget pose
defaults to identity since both skeletons share an A-pose family.
"""

import unreal


DEST_PACKAGE = "/Game/Characters/Player/IK"

# Source skeleton paths
SKEL_MIXAMO = "/Game/Mixamo/Raw/Standard_Idle_Skeleton"
SKEL_MANNY_CANDIDATES = [
    "/Game/Characters/Mannequins/Meshes/SK_Mannequin_Skeleton",
    "/Game/Characters/Manny/Meshes/SK_Mannequin_Skeleton",
]

# Mixamo X-Bot bone hierarchy (deterministic per character)
MIXAMO_CHAINS = {
    "Spine":   ("Spine", "Spine2"),
    "Head":    ("Neck",  "Head"),
    "ArmL":    ("LeftShoulder",  "LeftHand"),
    "ArmR":    ("RightShoulder", "RightHand"),
    "LegL":    ("LeftUpLeg",  "LeftToeBase"),
    "LegR":    ("RightUpLeg", "RightToeBase"),
}

# UE5 Manny bone names
MANNY_CHAINS = {
    "Spine":   ("spine_01", "spine_05"),
    "Head":    ("neck_01",  "head"),
    "ArmL":    ("clavicle_l", "hand_l"),
    "ArmR":    ("clavicle_r", "hand_r"),
    "LegL":    ("thigh_l", "ball_l"),
    "LegR":    ("thigh_r", "ball_r"),
}


def _find_manny_skeleton():
    for path in SKEL_MANNY_CANDIDATES:
        if unreal.EditorAssetLibrary.does_asset_exist(path):
            return path
    return None


def _build_ik_rig(name, skeleton_path, chains):
    """Create an IKRigDefinition asset with retarget chains. Returns (was_created, asset_path)."""
    asset_path = f"{DEST_PACKAGE}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        return False, asset_path

    skel = unreal.EditorAssetLibrary.load_asset(skeleton_path)
    if not skel:
        raise RuntimeError(f"skeleton not found: {skeleton_path}")

    tools = unreal.AssetToolsHelpers.get_asset_tools()
    # IKRigDefinitionFactory may not be Python-exposed in every UE version; create_asset can
    # accept None for the factory in some builds.
    factory = None
    try:
        factory = unreal.IKRigDefinitionFactory()
        factory.set_editor_property("target_skeleton_asset", skel)
    except Exception:
        pass

    rig = tools.create_asset(name, DEST_PACKAGE, unreal.IKRigDefinition, factory)
    if not rig:
        raise RuntimeError(f"create_asset failed for IK rig {name}")

    ctrl = unreal.IKRigController.get_controller(rig)
    ctrl.set_skeleton(skel)
    for chain_name, (start, end) in chains.items():
        try:
            ctrl.add_retarget_chain(chain_name, start, end, "")
        except Exception as e:
            # Soft-fail per-chain so partial setup doesn't lose the whole rig
            unreal.log_warning(f"[build_ik_rigs] chain {chain_name} ({start}->{end}) failed: {e}")

    unreal.EditorAssetLibrary.save_asset(asset_path)
    return True, asset_path


def _build_retargeter(name, source_rig_path, target_rig_path):
    asset_path = f"{DEST_PACKAGE}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        return False, asset_path

    tools = unreal.AssetToolsHelpers.get_asset_tools()
    rtg = tools.create_asset(name, DEST_PACKAGE, unreal.IKRetargeter, None)
    if not rtg:
        raise RuntimeError(f"create_asset failed for retargeter {name}")

    ctrl = unreal.IKRetargeterController.get_controller(rtg)
    src = unreal.EditorAssetLibrary.load_asset(source_rig_path)
    tgt = unreal.EditorAssetLibrary.load_asset(target_rig_path)
    if not src or not tgt:
        raise RuntimeError("source or target IK rig missing")

    ctrl.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE, src)
    ctrl.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET, tgt)
    unreal.EditorAssetLibrary.save_asset(asset_path)
    return True, asset_path


def run(args):
    result = {"created": [], "skipped": [], "failed": []}
    try:
        manny_skel_path = _find_manny_skeleton()
        if not manny_skel_path:
            result["failed"].append("UE5 Manny skeleton not found in any candidate path")
            return result

        if not unreal.EditorAssetLibrary.does_asset_exist(SKEL_MIXAMO):
            result["failed"].append(
                f"Mixamo skeleton not yet imported at {SKEL_MIXAMO}; run step 03 first"
            )
            return result

        created, _ = _build_ik_rig("IK_Mixamo", SKEL_MIXAMO, MIXAMO_CHAINS)
        (result["created"] if created else result["skipped"]).append("IK_Mixamo")

        created, _ = _build_ik_rig("IK_Manny", manny_skel_path, MANNY_CHAINS)
        (result["created"] if created else result["skipped"]).append("IK_Manny")

        created, _ = _build_retargeter(
            "RTG_MixamoToManny",
            f"{DEST_PACKAGE}/IK_Mixamo",
            f"{DEST_PACKAGE}/IK_Manny",
        )
        (result["created"] if created else result["skipped"]).append("RTG_MixamoToManny")
    except Exception as e:
        result["failed"].append(str(e))

    return result
