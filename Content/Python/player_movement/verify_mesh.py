"""Step 01 acceptance — verify BP_VSPlayer mesh + capsule + input bindings.

Returns a dict::

    {
        "ok": bool,
        "issues": [str, ...]    # human-readable list of what's wrong (empty when ok)
    }

Run via the bridge:
    POST /pof/python/run  {"module": "player_movement.verify_mesh", "function": "run"}
"""

import unreal


# Asset paths the step expects to be set up
BP_VSPLAYER_PATH = "/Game/VerticalSlice/BP_VSPlayer"
IMC_PATH = "/Game/Input/IMC_VerticalSlice"

# Input Actions that should be referenced by the IMC. IA_Sprint is created at runtime
# by ARPGPlayerController::CreateIA (not a .uasset), so we don't assert its file presence
# — we just rely on the controller's BeginPlay to materialize it.
REQUIRED_IA_ASSETS = [
    "/Game/Input/Actions/IA_Move",
    "/Game/Input/Actions/IA_Dodge",
]


def _load_blueprint_cdo(blueprint_path):
    """Load a Blueprint asset and return its CDO, or None if not found."""
    bp = unreal.EditorAssetLibrary.load_asset(blueprint_path)
    if not bp:
        return None
    # UBlueprint.generated_class is a METHOD in UE Python — call it to get the UClass.
    try:
        gen_class = bp.generated_class()
    except Exception:
        return None
    return unreal.get_default_object(gen_class) if gen_class else None


def run(args):
    issues = []

    # ── BP_VSPlayer present? ───────────────────────────────────────────────────
    if not unreal.EditorAssetLibrary.does_asset_exist(BP_VSPLAYER_PATH):
        issues.append(f"missing BP_VSPlayer at {BP_VSPLAYER_PATH}")
    else:
        cdo = _load_blueprint_cdo(BP_VSPLAYER_PATH)
        if not cdo:
            issues.append(f"BP_VSPlayer at {BP_VSPLAYER_PATH} has no CDO")
        else:
            # Mesh component populated?
            mesh_comp = None
            try:
                mesh_comp = cdo.get_editor_property("mesh")
            except Exception:
                pass
            if not mesh_comp:
                issues.append("BP_VSPlayer CDO has no mesh component")
            else:
                skel_mesh = None
                try:
                    skel_mesh = mesh_comp.get_editor_property("skeletal_mesh")
                except Exception:
                    pass
                if not skel_mesh:
                    issues.append("BP_VSPlayer.Mesh.SkeletalMesh is null")

            # Capsule sized?
            capsule = None
            try:
                capsule = cdo.get_editor_property("capsule_component")
            except Exception:
                pass
            if capsule:
                half_height = None
                try:
                    half_height = capsule.get_editor_property("capsule_half_height")
                except Exception:
                    pass
                if half_height is not None and abs(half_height - 90.0) > 5.0:
                    issues.append(f"BP_VSPlayer capsule half-height = {half_height}, expected 90")

    # ── IMC + key IAs present? ─────────────────────────────────────────────────
    if not unreal.EditorAssetLibrary.does_asset_exist(IMC_PATH):
        issues.append(f"missing IMC at {IMC_PATH}")

    for ia in REQUIRED_IA_ASSETS:
        if not unreal.EditorAssetLibrary.does_asset_exist(ia):
            issues.append(f"missing Input Action at {ia}")

    return {"ok": len(issues) == 0, "issues": issues}
