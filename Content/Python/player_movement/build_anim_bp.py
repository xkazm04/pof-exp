"""Step 08 — Author ABP_VSPlayer procedurally via UPoFAnimBPAuthoringLibrary.

Requires the PoFEditor module to be loaded (run step 07 first).
"""

import unreal


PACKAGE = "/Game/Characters/Player"
ASSET_NAME = "ABP_VSPlayer"
BP_VSPLAYER_PATH = "/Game/VerticalSlice/BP_VSPlayer"
BS_PATH = "/Game/Characters/Player/Animations/BS_Locomotion"

SKEL_CANDIDATES = [
    "/Game/Characters/Mannequins/Meshes/SK_Mannequin_Skeleton",
    "/Game/Characters/Manny/Meshes/SK_Mannequin_Skeleton",
]


def _find_skeleton():
    for path in SKEL_CANDIDATES:
        asset = unreal.EditorAssetLibrary.load_asset(path)
        if asset:
            return asset
    return None


def run(args):
    result = {"created": [], "skipped": [], "failed": []}

    skeleton = _find_skeleton()
    if not skeleton:
        result["failed"].append("UE5 Manny skeleton not found in any candidate path")
        return result

    bs = unreal.EditorAssetLibrary.load_asset(BS_PATH)
    if not bs:
        result["failed"].append(f"BS_Locomotion not found at {BS_PATH}; run step 06 first")
        return result

    lib = getattr(unreal, "PoFAnimBPAuthoringLibrary", None)
    if lib is None:
        result["failed"].append(
            "PoFAnimBPAuthoringLibrary not available — rebuild PoFEditor + restart editor (step 07)"
        )
        return result

    asset_path = f"{PACKAGE}/{ASSET_NAME}"
    pre_existed = unreal.EditorAssetLibrary.does_asset_exist(asset_path)

    abp = lib.create_anim_blueprint(skeleton, PACKAGE, ASSET_NAME)
    if not abp:
        result["failed"].append("create_anim_blueprint returned null")
        return result

    if not lib.add_state_machine(abp, "Locomotion"):
        result["failed"].append("add_state_machine(Locomotion) failed")
        return result

    if not lib.add_blend_space_state(abp, "Locomotion", "Strafe", bs, "Speed", "Direction"):
        result["failed"].append("add_blend_space_state failed")
        return result

    if not lib.add_default_slot(abp, "DefaultSlot"):
        result["failed"].append("add_default_slot failed")
        return result

    if not lib.connect_state_machine_to_output_pose(abp, "Locomotion", "DefaultSlot"):
        result["failed"].append("connect_state_machine_to_output_pose failed")
        return result

    if not lib.compile_and_save(abp):
        result["failed"].append("compile_and_save failed (check compile log)")
        return result

    # Wire ABP into BP_VSPlayer.Mesh.AnimClass
    try:
        bp_player = unreal.EditorAssetLibrary.load_asset(BP_VSPLAYER_PATH)
        if bp_player:
            cdo = unreal.get_default_object(bp_player.generated_class)
            mesh = cdo.get_editor_property("mesh") if cdo else None
            if mesh:
                mesh.set_editor_property("anim_class", abp.generated_class)
                unreal.EditorAssetLibrary.save_asset(BP_VSPLAYER_PATH)
    except Exception as e:
        # Non-fatal: AnimBP itself was built. Surface the wiring failure for the user.
        result["failed"].append(f"BP_VSPlayer AnimClass wiring failed: {e}")

    (result["skipped"] if pre_existed else result["created"]).append(ASSET_NAME)
    return result
