"""Create the lightsaber blade material: unlit, HDR-emissive blue (blooms). Saved to /Game/FX."""
import unreal


def _make(name, color):
    pkg = "/Game/FX"
    full = pkg + "/" + name
    eal = unreal.EditorAssetLibrary
    if eal.does_asset_exist(full):
        eal.delete_asset(full)
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    mat = tools.create_asset(name, pkg, unreal.Material, unreal.MaterialFactoryNew())
    if not mat:
        unreal.log_error("SABER_MAT: create failed for %s" % name)
        return False
    try:
        mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    except Exception as e:
        unreal.log_warning("SABER_MAT: shading_model set failed (%s) — leaving lit+emissive" % e)
    mel = unreal.MaterialEditingLibrary
    col = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -350, 0)
    col.set_editor_property("constant", color)  # HDR (>1) values bloom
    mel.connect_material_property(col, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(mat)
    eal.save_asset(full)
    unreal.log("SABER_MAT: saved %s" % full)
    return True


def _make_lit(name, base, rough):
    """A LIT material (dark Sith body) — base colour + roughness, so it shades like a
    character instead of a flat emissive blob."""
    pkg = "/Game/FX"
    full = pkg + "/" + name
    eal = unreal.EditorAssetLibrary
    if eal.does_asset_exist(full):
        eal.delete_asset(full)
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    mat = tools.create_asset(name, pkg, unreal.Material, unreal.MaterialFactoryNew())
    if not mat:
        unreal.log_error("SABER_MAT: create failed for %s" % name)
        return False
    # A character body material MUST declare skeletal-mesh usage or the engine rejects it at
    # runtime and falls back to the default grey material ("missing usage flag SkeletalMesh").
    try:
        mat.set_editor_property("used_with_skeletal_mesh", True)
    except Exception as e:
        unreal.log_warning("SABER_MAT: could not set used_with_skeletal_mesh (%s)" % e)
    mel = unreal.MaterialEditingLibrary
    bc = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -350, 0)
    bc.set_editor_property("constant", base)
    mel.connect_material_property(bc, "", unreal.MaterialProperty.MP_BASE_COLOR)
    r = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -350, 220)
    r.set_editor_property("r", rough)
    mel.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(mat)
    eal.save_asset(full)
    unreal.log("SABER_MAT: saved %s" % full)
    return True


def main():
    unreal.log("SABER_MAT: BEGIN")
    ok_blue = _make("M_Saber_Blue", unreal.LinearColor(1.2, 5.0, 18.0, 1.0))   # Jedi
    ok_red = _make("M_Saber_Red", unreal.LinearColor(18.0, 1.0, 0.6, 1.0))     # Sith
    ok_body = _make_lit("M_Sith_Body", unreal.LinearColor(0.035, 0.035, 0.045, 1.0), 0.55)  # dark robes
    unreal.log("[gate] RESULT=%s" % ("PASS" if (ok_blue and ok_red and ok_body) else "FAIL"))


main()
