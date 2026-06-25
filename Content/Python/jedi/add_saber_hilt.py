"""
add_saber_hilt.py — give the lightsaber a real hilt.

The blade is the inherited WeaponMesh (a thin glowing cylinder). Add a short
dark-metal cylinder as a CHILD of WeaponMesh, sized + placed so it surrounds the
blade's base — the opaque hilt (5cm dia) hides the blade's lowest ~7cm and the
glowing blade reads as emerging from the hilt top, gripped in hand_r.

Child of WeaponMesh inherits its (0.036,0.036,0.95) scale, so the hilt is
counter-scaled back to real-world cm. Added via SubobjectDataSubsystem; idempotent
(reconfigures an existing SaberHilt instead of adding a second).
Writes Saved/add_saber_hilt.json.
"""
import json
import unreal

asset_lib = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
BP_JEDI = "/Game/Characters/Jedi/BP_JediPlayer"
CYLINDER = "/Engine/BasicShapes/Cylinder"
OUT = {"steps": []}


def _log(m):
    unreal.log_warning("[hilt] " + str(m))
    OUT["steps"].append(str(m))


def make_hilt_material():
    name, pkg = "M_SaberHilt", "/Game/FX"
    full = pkg + "/" + name
    if asset_lib.does_asset_exist(full):
        return asset_lib.load_asset(full)
    mat = tools.create_asset(name, pkg, unreal.Material, unreal.MaterialFactoryNew())
    bc = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -350, 0)
    bc.set_editor_property("constant", unreal.LinearColor(0.025, 0.025, 0.03, 1.0))  # dark metal
    mel.connect_material_property(bc, "", unreal.MaterialProperty.MP_BASE_COLOR)
    m = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -350, 150)
    m.set_editor_property("r", 1.0)
    mel.connect_material_property(m, "", unreal.MaterialProperty.MP_METALLIC)
    r = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -350, 300)
    r.set_editor_property("r", 0.35)
    mel.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(mat)
    asset_lib.save_asset(full)
    _log("M_SaberHilt created")
    return mat


def find_handle(sub, bp, name_prefix, cls):
    handles = sub.k2_gather_subobject_data_for_blueprint(bp)
    bfl = unreal.SubobjectDataBlueprintFunctionLibrary
    for h in handles or []:
        data = sub.k2_find_subobject_data_from_handle(h)
        if data is None:
            continue
        obj = bfl.get_associated_object(data)
        if obj is not None and isinstance(obj, cls) and obj.get_name().startswith(name_prefix):
            return h, obj
    return None, None


def main():
    _log("=== ADD SABER HILT START ===")
    bp = asset_lib.load_asset(BP_JEDI)
    hilt_mat = make_hilt_material()
    cyl = asset_lib.load_asset(CYLINDER)
    sub = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    bfl = unreal.SubobjectDataBlueprintFunctionLibrary

    weapon_handle, weapon = find_handle(sub, bp, "WeaponMesh", unreal.StaticMeshComponent)
    if weapon_handle is None:
        _log("[gate] RESULT=FAIL (WeaponMesh not found)")
        return

    # Existing hilt? reconfigure; else add a child of WeaponMesh.
    _, existing = find_handle(sub, bp, "SaberHilt", unreal.StaticMeshComponent)
    hilt = existing
    if hilt is None:
        params = unreal.AddNewSubobjectParams()
        params.set_editor_property("parent_handle", weapon_handle)
        params.set_editor_property("new_class", unreal.StaticMeshComponent)
        params.set_editor_property("blueprint_context", bp)
        new_handle, fail = sub.add_new_subobject(params)
        if fail and str(fail):
            _log("add_new_subobject fail: %s" % fail)
        sub.rename_subobject(new_handle, unreal.Text.cast("SaberHilt"))
        data = sub.k2_find_subobject_data_from_handle(new_handle)
        hilt = bfl.get_associated_object(data)
        _log("added SaberHilt child of WeaponMesh")
    else:
        _log("SaberHilt exists — reconfiguring")

    # Counter-scale (parent scale 0.036,0.036,0.95) -> ~5cm dia, ~15cm long.
    hilt.set_editor_property("static_mesh", cyl)
    hilt.set_editor_property("override_materials", [hilt_mat])
    try:
        hilt.set_material(0, hilt_mat)
    except Exception:
        pass
    hilt.set_editor_property("relative_location", unreal.Vector(0, 0, -50.0))  # at blade base
    hilt.set_editor_property("relative_scale3d", unreal.Vector(1.4, 1.4, 0.16))
    hilt.set_editor_property("relative_rotation", unreal.Rotator(0, 0, 0))

    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    asset_lib.save_asset(BP_JEDI)
    _log("compiled + saved %s" % BP_JEDI)

    OUT["result"] = "PASS"
    out_path = unreal.Paths.combine([unreal.Paths.project_saved_dir(), "add_saber_hilt.json"])
    with open(out_path, "w", encoding="utf-8") as fh:
        json.dump(OUT, fh, indent=2)
    _log("[gate] RESULT=PASS")


main()
