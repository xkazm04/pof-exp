"""Step 08 — Author ABP_VSPlayer procedurally via UPoFAnimBPAuthoringLibrary.

Requires the PoFEditor module to be loaded (run step 07 first).
"""

import unreal


PACKAGE = "/Game/Characters/Player"
ASSET_NAME = "ABP_VSPlayer"
BP_VSPLAYER_PATH = "/Game/VerticalSlice/BP_VSPlayer"
BS_PATH = "/Game/Characters/Player/Animations/BS_Locomotion"

SKEL_CANDIDATES = [
    "/MoverTests/Characters/Mannequins/Meshes/SK_Mannequin",
    "/MoverExamples/Characters/Mannequins/Meshes/SK_Mannequin",
    "/Game/Characters/Mannequins/Meshes/SK_Mannequin_Skeleton",
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

    # Parent the ABP to UARPGAnimInstance so it inherits the C++-computed locomotion
    # vars (Speed cm/s, Direction -180..180) — the blend space is driven by movement
    # with zero EventGraph logic.
    abp = lib.create_anim_blueprint(skeleton, PACKAGE, ASSET_NAME, "/Script/PoF.ARPGAnimInstance")
    if not abp:
        result["failed"].append("create_anim_blueprint returned null")
        return result

    # Locomotion graph: BlendSpacePlayer -> Slot -> Output (no state machine — the
    # state-machine path leaves an unconnected entry state that compiles with warnings).
    if not lib.add_blend_space_player_to_output(abp, bs, "Speed", "Direction", "DefaultSlot"):
        result["failed"].append("add_blend_space_player_to_output failed")
        return result

    if not lib.compile_and_save(abp):
        result["failed"].append("compile_and_save failed (check compile log)")
        return result

    # Wire ABP into BP_VSPlayer.Mesh.AnimClass so the player actually uses it.
    # NOTE: in UE Python, Blueprint.generated_class is a METHOD — call it.
    try:
        bp_player = unreal.EditorAssetLibrary.load_asset(BP_VSPLAYER_PATH)
        if bp_player:
            gen_class = bp_player.generated_class()
            cdo = unreal.get_default_object(gen_class) if gen_class else None
            mesh = cdo.get_editor_property("mesh") if cdo else None
            abp_class = abp.generated_class()
            if mesh and abp_class:
                mesh.set_editor_property("animation_mode", unreal.AnimationMode.ANIMATION_BLUEPRINT)
                mesh.set_editor_property("anim_class", abp_class)
                unreal.EditorAssetLibrary.save_asset(BP_VSPLAYER_PATH)
                result["created"].append("BP_VSPlayer.AnimClass->ABP_VSPlayer")
            else:
                result["failed"].append("BP_VSPlayer mesh or ABP generated class unavailable")
    except Exception as e:  # noqa: BLE001
        result["failed"].append(f"BP_VSPlayer AnimClass wiring failed: {e}")

    (result["skipped"] if pre_existed else result["created"]).append(ASSET_NAME)
    return result
