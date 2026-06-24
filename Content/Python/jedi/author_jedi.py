"""
author_jedi.py — Stream 2: turn BP_JediPlayer into a Jedi.

Configures the Phase-0 BP_JediPlayer stub (a subclass of BP_VSPlayer) WITHOUT
editing ARPGCharacterBase:
  1. Robe materials (M_JediRobe warm brown + M_JediTunic tan) -> CharacterMesh0
     override slots, so the silver Manny reads as a robed Jedi.
  2. Lightsaber: WeaponMesh static mesh -> a thin glowing blade (engine Cylinder
     + M_Saber_Blue, unlit HDR-emissive blue). The blade reuses the inherited
     WeaponMesh grip ROTATION (already tuned for hand_r by the duel's RuneSword)
     and is offset along its local +Z so the base emerges from the hand.
  3. A short dark hilt (SaberHilt child static-mesh component) so the saber reads
     as held, not a floating beam.

Inherited NATIVE components (CharacterMesh0, WeaponMesh) serialize on the
generated-class CDO, so every write is applied to BOTH the SCS template AND the
CDO component (the setup_characters_ue.py lesson).

Idempotent: re-running rebuilds the materials and re-applies config.
Writes Saved/author_jedi.json for verification.
"""
import json
import math
import unreal

asset_lib = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()

BP_JEDI = "/Game/Characters/Jedi/BP_JediPlayer"
CYLINDER = "/Engine/BasicShapes/Cylinder"
M_SABER_BLUE = "/Game/FX/M_Saber_Blue"
FX_DIR = "/Game/FX"

OUT = {"steps": []}


def _log(m):
    unreal.log_warning("[author_jedi] " + str(m))
    OUT["steps"].append(str(m))


def make_lit_material(name, base_rgb, rough):
    """A lit body material declaring SkeletalMesh usage (or UE falls back to grey)."""
    full = FX_DIR + "/" + name
    if asset_lib.does_asset_exist(full):
        asset_lib.delete_asset(full)
    mat = tools.create_asset(name, FX_DIR, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("used_with_skeletal_mesh", True)
    bc = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -350, 0)
    bc.set_editor_property("constant", unreal.LinearColor(*base_rgb, 1.0))
    mel.connect_material_property(bc, "", unreal.MaterialProperty.MP_BASE_COLOR)
    r = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -350, 220)
    r.set_editor_property("r", rough)
    mel.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(mat)
    asset_lib.save_asset(full)
    _log("material %s saved (base=%s rough=%.2f)" % (full, base_rgb, rough))
    return mat


def rotate_vec(rotator, v):
    """Rotate FVector v by FRotator using quaternion (yaw/pitch/roll degrees)."""
    q = rotator.quaternion()
    return q.rotate_vector(v)


def apply_to_template_and_cdo(bp, comp_finder, apply_fn):
    """Run apply_fn(component) on both the SCS template component and the CDO component."""
    # Template (subobject) component
    sub = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    handles = sub.k2_gather_subobject_data_for_blueprint(bp)
    bfl = unreal.SubobjectDataBlueprintFunctionLibrary
    applied = 0
    for h in handles or []:
        data = sub.k2_find_subobject_data_from_handle(h)
        if data is None:
            continue
        obj = bfl.get_associated_object(data)
        if obj is not None and comp_finder(obj):
            apply_fn(obj)
            applied += 1
    # CDO component
    try:
        gen = bp.generated_class()
        cdo = unreal.get_default_object(gen) if gen else None
        if cdo is not None:
            for prop in ("mesh", "weapon_mesh"):
                try:
                    c = cdo.get_editor_property(prop)
                except Exception:
                    c = None
                if c is not None and comp_finder(c):
                    apply_fn(c)
                    applied += 1
    except Exception as e:
        _log("CDO apply error: %s" % e)
    return applied


def main():
    _log("=== AUTHOR JEDI START ===")
    bp = asset_lib.load_asset(BP_JEDI)
    if bp is None:
        _log("[gate] RESULT=FAIL (BP_JediPlayer missing)")
        return

    # --- 1. Robe materials ---------------------------------------------------
    robe = make_lit_material("M_JediRobe", (0.30, 0.185, 0.095), 0.60)   # medium brown robe
    tunic = make_lit_material("M_JediTunic", (0.64, 0.54, 0.38), 0.52)   # light sand tunic
    robe_path, tunic_path = robe.get_path_name(), tunic.get_path_name()

    # --- 2. Read the original WeaponMesh grip transform ----------------------
    # Read from the PARENT BP_VSPlayer (never modified) so re-runs are stable —
    # reading our own CDO would return the already-offset blade and compound it.
    grip_loc = unreal.Vector(7.96, -0.57, 0.49)
    grip_rot = unreal.Rotator(0, 0, 0)
    try:
        parent_bp = asset_lib.load_asset("/Game/VerticalSlice/BP_VSPlayer")
        pcdo = unreal.get_default_object(parent_bp.generated_class())
        wm = pcdo.get_editor_property("weapon_mesh")
        if wm:
            grip_loc = wm.get_editor_property("relative_location")
            grip_rot = wm.get_editor_property("relative_rotation")
    except Exception as e:
        _log("read grip transform failed (%s) — using defaults" % e)
    _log("grip loc=(%.2f,%.2f,%.2f) rot=(p%.1f y%.1f r%.1f)" % (
        grip_loc.x, grip_loc.y, grip_loc.z, grip_rot.pitch, grip_rot.yaw, grip_rot.roll))

    # --- 3. Saber blade transform -------------------------------------------
    # Engine Cylinder: 100cm tall along local Z, radius 50cm, pivot at CENTER.
    # Blade ~ 95cm long, ~3.6cm diameter.
    blade_scale = unreal.Vector(0.036, 0.036, 0.95)   # 3.6cm dia, 95cm long
    half_len = 50.0 * blade_scale.z                    # cm from center to tip in component space
    # Push the cylinder so its BASE sits at the grip and it extends along the
    # (rotated) local +Z that GetSaberSegment treats as hilt->tip.
    blade_offset = rotate_vec(grip_rot, unreal.Vector(0, 0, half_len))
    blade_loc = unreal.Vector(grip_loc.x + blade_offset.x,
                              grip_loc.y + blade_offset.y,
                              grip_loc.z + blade_offset.z)
    cyl = asset_lib.load_asset(CYLINDER)
    saber_mat = asset_lib.load_asset(M_SABER_BLUE)

    def is_weapon(o):
        return isinstance(o, unreal.StaticMeshComponent) and o.get_name().startswith("WeaponMesh")

    def config_weapon(wm):
        wm.set_editor_property("static_mesh", cyl)
        wm.set_editor_property("override_materials", [saber_mat])
        try:
            wm.set_material(0, saber_mat)
        except Exception:
            pass
        wm.set_editor_property("relative_location", blade_loc)
        wm.set_editor_property("relative_rotation", grip_rot)
        wm.set_editor_property("relative_scale3d", blade_scale)

    n_wm = apply_to_template_and_cdo(bp, is_weapon, config_weapon)
    _log("WeaponMesh configured on %d instance(s): blade loc=(%.1f,%.1f,%.1f) scale=%s"
         % (n_wm, blade_loc.x, blade_loc.y, blade_loc.z, blade_scale))

    # --- 4. Robe material override on the body mesh -------------------------
    def is_body(o):
        return isinstance(o, unreal.SkeletalMeshComponent) and o.get_name().startswith("CharacterMesh0")

    def config_body(m):
        m.set_editor_property("override_materials", [tunic, robe])  # slot0 tunic, slot1 robe
        try:
            m.set_material(0, tunic)
            m.set_material(1, robe)
        except Exception:
            pass

    n_body = apply_to_template_and_cdo(bp, is_body, config_body)
    _log("Body robe materials applied on %d instance(s)" % n_body)

    # --- 5. Compile + save ---------------------------------------------------
    # NB: do NOT compile AFTER setting override_materials on inherited components
    # (a compile resets them). The CDO writes above persist through save.
    asset_lib.save_asset(BP_JEDI)
    _log("saved %s" % BP_JEDI)

    OUT["robe"] = robe_path
    OUT["tunic"] = tunic_path
    OUT["blade_loc"] = [blade_loc.x, blade_loc.y, blade_loc.z]
    OUT["blade_scale"] = [blade_scale.x, blade_scale.y, blade_scale.z]
    OUT["weapon_instances"] = n_wm
    OUT["body_instances"] = n_body
    OUT["result"] = "PASS" if (n_wm > 0 and n_body > 0) else "FAIL"

    out_path = unreal.Paths.combine([unreal.Paths.project_saved_dir(), "author_jedi.json"])
    with open(out_path, "w", encoding="utf-8") as fh:
        json.dump(OUT, fh, indent=2)
    _log("wrote " + out_path)
    _log("[gate] RESULT=%s" % OUT["result"])


main()
