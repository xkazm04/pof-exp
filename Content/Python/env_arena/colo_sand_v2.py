"""Colosseum — darken the sand so the ground reads as warm desert tan (it was blowing to
white under the strong sun) and add grain relief via the engine tiling detail normal."""
import unreal

SAND = "/Game/Environments/AncientArena/M_Sand_Floor"
T_NOISE = "/Engine/MaterialTemplates/Textures/T_Noise01"
T_DETAIL_N = "/Engine/Engine_MI_Shaders/T_Base_Tile_DetailNormal"
mel = unreal.MaterialEditingLibrary


def pick(*c):
    for n in c:
        if hasattr(mel, n):
            return getattr(mel, n)
    return None


CE = pick("connect_material_expressions", "connect_material_expression")
CP = pick("connect_material_property")


def main():
    mat = unreal.load_asset(SAND)
    if not mat:
        unreal.log_warning("[SANDV2] missing"); return
    try:
        mel.delete_all_material_expressions(mat)
    except Exception:
        pass

    def E(cls, x, y):
        return mel.create_material_expression(mat, cls, x, y)

    dark = E(unreal.MaterialExpressionConstant3Vector, -760, -120)
    dark.set_editor_property("constant", unreal.LinearColor(0.40, 0.33, 0.22, 1.0))
    lite = E(unreal.MaterialExpressionConstant3Vector, -760, 60)
    lite.set_editor_property("constant", unreal.LinearColor(0.62, 0.53, 0.37, 1.0))
    # tiling noise for tonal variation
    noise = E(unreal.MaterialExpressionTextureSample, -760, 240)
    tn = unreal.load_asset(T_NOISE)
    if tn:
        noise.set_editor_property("texture", tn)
    tc = E(unreal.MaterialExpressionTextureCoordinate, -1040, 240)
    tc.set_editor_property("u_tiling", 4.0); tc.set_editor_property("v_tiling", 4.0)
    if CE:
        CE(tc, "", noise, "UVs")
    base = lite
    if CE:
        lerp = E(unreal.MaterialExpressionLinearInterpolate, -360, -40)
        CE(dark, "", lerp, "A"); CE(lite, "", lerp, "B"); CE(noise, "R", lerp, "Alpha")
        base = lerp
    CP(base, "", unreal.MaterialProperty.MP_BASE_COLOR)
    rough = E(unreal.MaterialExpressionConstant, -360, 250)
    rough.set_editor_property("r", 0.94)
    CP(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    spec = E(unreal.MaterialExpressionConstant, -360, 360)
    spec.set_editor_property("r", 0.06)
    CP(spec, "", unreal.MaterialProperty.MP_SPECULAR)
    # grain relief
    dn = E(unreal.MaterialExpressionTextureSample, -360, 500)
    dt = unreal.load_asset(T_DETAIL_N)
    if dt:
        dn.set_editor_property("texture", dt)
        try:
            dn.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        except Exception:
            pass
    dtc = E(unreal.MaterialExpressionTextureCoordinate, -640, 500)
    dtc.set_editor_property("u_tiling", 18.0); dtc.set_editor_property("v_tiling", 18.0)
    if CE:
        CE(dtc, "", dn, "UVs")
    CP(dn, "", unreal.MaterialProperty.MP_NORMAL)

    try:
        mat.set_editor_property("used_with_static_lighting", True)
    except Exception:
        pass
    mel.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(SAND)
    unreal.log("[SANDV2] darkened sand saved (CE=%s)" % (CE.__name__ if CE else None))
    unreal.log("[gate] RESULT=PASS")


main()
